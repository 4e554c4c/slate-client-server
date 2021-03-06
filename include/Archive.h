#ifndef SLATE_ARCHIVE_H
#define SLATE_ARCHIVE_H

#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <string>

///Check whether a string has only valid base64 characters
bool sanityCheckBase64(const std::string& str);

///Decode base64 encoded data
std::string decodeBase64(const std::string& coded);

///Encode data to base64
std::string encodeBase64(const std::string& raw);

///decompress gzipped data from one stream to another
void gzipDecompress(std::istream& src, std::ostream& dest);

///compress gzipped data from one stream to another
void gzipCompress(std::istream& src, std::ostream& dest);

//A simple interface for reading a tarball
//files are read in on demand, and can be dropped from memory when no longer needed
//Once dropped, a file cannot be retrieved again
class TarReader{
public:
	class missing_file_exception : public std::exception{
	private:
		std::string message;
	public:
		explicit missing_file_exception(const std::string& s):message(s){}
		~missing_file_exception() throw() {}
		const char* what() const throw (){
			return(message.c_str());
		}
	};
	
	///A simple description of a file which has been stored in a tar archive,
	///consisting of the file's type and contents
	class FileRecord{
	public:
		enum fileType{REGULAR_FILE=0, 
			HARD_LINK=1, 
			SYMBOLIC_LINK=2, 
			CHARACTER_DEVICE=3, 
			BLOCK_DEVICE=4, 
			DIRECTORY=5,
			FIFO=6,
			RESERVED=7
		};
	private:
		fileType type;
		std::string data;
		int mode;
	public:
		FileRecord();
		FileRecord(fileType t, int m=0644);
		FileRecord(fileType t, const std::string& d, int m=0644);
		FileRecord(fileType t, unsigned long long dataSize, std::istream& dataSrc, int m=0644);
		
		fileType getType() const{ return type; }
		int getMode() const{ return mode; }
		
		const std::string& getData() const;
		
		unsigned long long getFileSize() const;
	};
	
	typedef std::map<std::string,FileRecord>::const_iterator iterator;
	typedef std::map<std::string,FileRecord>::const_reverse_iterator reverse_iterator;
	
	TarReader(std::istream& source);
	
	std::unique_ptr<std::istream> streamForFile(const std::string& name);
	const std::string& stringForFile(const std::string& name);
	FileRecord::fileType typeForFile(const std::string& name);
	int modeForFile(const std::string& name);
	std::string nextFile();
	std::string nextFileOfType(FileRecord::fileType type);
	void dropFile(const std::string& name);
	bool eof() const;
	void extractToFileSystem(const std::string& prefix, bool dropAfterExtracting=true);
	
private:
	std::string readFiles(const std::string& target);
	
	std::istream& src;
	std::map<std::string,FileRecord> files;
	bool fileEnded;
};

class TarWriter{
public:
	TarWriter(std::ostream& s):sink(s),ended(false){}
	~TarWriter(){ endStream(); }
	void appendFile(const std::string& filepath, const std::string& data);
	void appendDirectory(const std::string& path);
	void appendSymLink(const std::string& filepath, const std::string& linkTarget);
	///Must be called to write the (empty) footer records which signal the end 
	///of the tr stream. Called automatically by the destructor, but may be 
	///called manually earlier. 
	void endStream();
private:
	std::ostream& sink;
	bool ended;
};

void recursivelyArchive(std::string basePath, TarWriter& writer, bool stripPrefix=true);

#endif //SLATE_ARCHIVE_H
