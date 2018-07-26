#include "test.h"

#include <set>

#include <Utilities.h>

TEST(UnauthenticatedFindUser){
	using namespace httpRequests;
	TestContext tc;
	
	//try looking up a user with no authentication
	//doesn't matter whether the user is real since this should be rejected on other grounds
	auto addResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/find_user?globus_id=abc");
	ENSURE_EQUAL(addResp.status,403,
				 "Requests to look up users without authentication should be rejected");
	
	//try looking up a user with invalid authentication
	addResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/find_user?token=00112233-4455-6677-8899-aabbccddeeff&globus_id=abc");
	ENSURE_EQUAL(addResp.status,403,
				 "Requests to look up users with invalid authentication should be rejected");
}

TEST(FindUser){
	using namespace httpRequests;
	TestContext tc;
	
	const std::string adminKey=getPortalToken();
	const std::string globusID="bobs_globus_id";
	
	std::string uid;
	{ //create a user
		rapidjson::Document request(rapidjson::kObjectType);
		auto& alloc = request.GetAllocator();
		request.AddMember("apiVersion", "v1alpha1", alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "Bob", alloc);
		metadata.AddMember("email", "bob@place.com", alloc);
		metadata.AddMember("admin", false, alloc);
		metadata.AddMember("globusID", globusID, alloc);
		request.AddMember("metadata", metadata, alloc);
		auto createResp=httpPost(tc.getAPIServerURL()+"/v1alpha1/users?token="+adminKey,to_string(request));
		ENSURE_EQUAL(createResp.status,200,"User creation request should succeed");
		rapidjson::Document createData;
		createData.Parse(createResp.body);
		uid=createData["metadata"]["id"].GetString();
	}
	
	auto schema=loadSchema("../../slate-portal-api-spec/FindUserResultSchema.json");

	std::string tok;
	{ //look up the user by globus ID
		auto infoResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/find_user?globus_id="+globusID+"&token="+adminKey);
		ENSURE_EQUAL(infoResp.status,200,"Looking up user should succeed");
		rapidjson::Document data;
		data.Parse(infoResp.body);
		ENSURE_CONFORMS(data,schema);
		ENSURE_EQUAL(data["metadata"]["id"].GetString(),uid,"Result should be for the cirrect user id");
		tok=data["metadata"]["access_token"].GetString();
	}
	
	{ //check that the access token we got works
		auto infoResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/users/"+uid+"?token="+tok);
		ENSURE_EQUAL(infoResp.status,200,"Returned access token should be valid for fetching user's info");
	}
}

TEST(FindNonexistentUser){
	using namespace httpRequests;
	TestContext tc;
	
	const std::string adminKey=getPortalToken();
	const std::string globusID="bobs_globus_id";
	
	{ //look up an invalid globus ID
		auto infoResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/find_user?globus_id="+globusID+"&token="+adminKey);
		ENSURE_EQUAL(infoResp.status,404,"Look up request for nonexistent user should fail");
	}
}

TEST(FindUserMissingID){
	using namespace httpRequests;
	TestContext tc;
	
	const std::string adminKey=getPortalToken();
	
	{ //look up without an target specified
		auto infoResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/find_user?token="+adminKey);
		ENSURE_EQUAL(infoResp.status,400,"Look up request without a target ID should fail");
	}
}

TEST(NonAdminFindUser){
	using namespace httpRequests;
	TestContext tc;
	
	const std::string adminKey=getPortalToken();
	const std::string globusID="bobs_globus_id";
	
	std::string uid;
	std::string tok;
	{ //create a user
		rapidjson::Document request(rapidjson::kObjectType);
		auto& alloc = request.GetAllocator();
		request.AddMember("apiVersion", "v1alpha1", alloc);
		rapidjson::Value metadata(rapidjson::kObjectType);
		metadata.AddMember("name", "Bob", alloc);
		metadata.AddMember("email", "bob@place.com", alloc);
		metadata.AddMember("admin", false, alloc);
		metadata.AddMember("globusID", globusID, alloc);
		request.AddMember("metadata", metadata, alloc);
		auto createResp=httpPost(tc.getAPIServerURL()+"/v1alpha1/users?token="+adminKey,to_string(request));
		ENSURE_EQUAL(createResp.status,200,"User creation request should succeed");
		rapidjson::Document createData;
		createData.Parse(createResp.body);
		uid=createData["metadata"]["id"].GetString();
		tok=createData["metadata"]["access_token"].GetString();
	}
	
	{ //look up the user by globus ID
		auto infoResp=httpGet(tc.getAPIServerURL()+"/v1alpha1/find_user?globus_id="+globusID+"&token="+tok);
		ENSURE_EQUAL(infoResp.status,403,"Non-admin requests to this method should be rejected");
	}
}