//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////
// $URL: http://svn.rebarp.se/svn/RME/trunk/source/filehandle.h $
// $Id: filehandle.h 306 2010-02-24 10:38:25Z admin $

#ifndef RME_FILEHANDLE_H_
#define RME_FILEHANDLE_H_

#include "definitions.h"

#include <stdexcept>
#include <string>
#include <stack>
#include <stdio.h>

#if __VISUALC__
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned int uint;
#else
typedef unsigned int uint;
#endif

#ifndef FORCEINLINE
#   ifdef _MSV_VER
#       define FORCEINLINE __forceinline
#   else
#       define FORCEINLINE inline
#   endif
#endif

enum FileHandleError {
	FILE_NO_ERROR,
	FILE_COULD_NOT_OPEN,
	FILE_STRING_TOO_LONG,
	FILE_READ_ERROR,
	FILE_WRITE_ERROR,
	FILE_SYNTAX_ERROR,
	FILE_PREMATURE_END,
};

enum NodeType {
	NODE_START = 0xfe,
	NODE_END = 0xff,
	ESCAPE_CHAR = 0xfd,
};

class FileHandle {
public:
	FileHandle() : error_code(FILE_NO_ERROR), file(NULL) {}
	virtual ~FileHandle() {close();}

	virtual void close();
	virtual bool isOpen() {return file != NULL;}
	virtual bool isOk() {return isOpen() && error_code == FILE_NO_ERROR && ferror(file) == 0;}
	std::string getErrorMessage();
public:
	FileHandleError error_code;
	FILE* file;
};

class FileReadHandle : public FileHandle {
public:
	FileReadHandle(std::string name);
	virtual ~FileReadHandle();

	FORCEINLINE bool getU8(uint8_t& u8) {return getType(u8);}
	FORCEINLINE bool getByte(uint8_t& u8) {return getType(u8);}
	FORCEINLINE bool getU16(uint16_t& u16) {return getType(u16);}
	FORCEINLINE bool getU32(uint32_t& u32) {return getType(u32);}
	bool getRAW(uint8_t* ptr, size_t sz);
	bool getRAW(std::string& str, size_t sz);
	bool getString(std::string& str);
	bool getLongString(std::string& str);

	virtual void close();
	bool seek(size_t offset);
	bool seekRelative(size_t offset);
	FORCEINLINE void skip(size_t offset) {seekRelative(offset);}
	size_t size() {return file_size;}
	size_t tell() {if(file) return ftell(file); return 0;}
protected:
	size_t file_size;

	template<class T>
	bool getType(T& ref) {
		fread(&ref, sizeof(ref), 1, file);
		return ferror(file) == 0;
	}
};

class NodeFileReadHandle;
class DiskNodeFileReadHandle;
class MemoryNodeFileReadHandle;

class BinaryNode {
public:
	BinaryNode(NodeFileReadHandle* file, BinaryNode* parent);
	~BinaryNode();

	FORCEINLINE bool getU8(uint8_t& u8) {return getType(u8);}
	FORCEINLINE bool getByte(uint8_t& u8) {return getType(u8);}
	FORCEINLINE bool getU16(uint16_t& u16) {return getType(u16);}
	FORCEINLINE bool getU32(uint32_t& u32) {return getType(u32);}
	FORCEINLINE bool skip(size_t sz) {
		if(read_offset + sz > data.size()) {
			read_offset = data.size();
			return false;
		}
		read_offset += sz;
		return true;
	}
	bool getRAW(uint8_t* ptr, size_t sz);
	bool getRAW(std::string& str, size_t sz);
	bool getString(std::string& str);
	bool getLongString(std::string& str);

	BinaryNode* getChild();
	// Returns this on success, NULL on failure
	BinaryNode* advance();
protected:
	template<class T>
	bool getType(T& ref) {
		if(read_offset + sizeof(ref) > data.size()) {
			read_offset = data.size();
			return false;
		}
		ref = *(T*)(data.data()+read_offset);

		read_offset += sizeof(ref);
		return true;
	}

	void load();
	std::string data;
	size_t read_offset;
	NodeFileReadHandle* file;
	BinaryNode* parent;
	BinaryNode* child;

	friend class DiskNodeFileReadHandle;
	friend class MemoryNodeFileReadHandle;
};

class NodeFileReadHandle : public FileHandle {
public:
	NodeFileReadHandle();
	virtual ~NodeFileReadHandle();
	
	virtual BinaryNode* getRootNode() = 0;

	virtual size_t size() = 0;
	virtual size_t tell() = 0;
protected:
	BinaryNode* getNode(BinaryNode* parent);
	void freeNode(BinaryNode* node);
	// Returns false when end-of-file is reached
	virtual bool renewCache() = 0;
	
	bool last_was_start;
	uint8_t* cache;
	size_t cache_size;
	size_t cache_length;
	size_t local_read_index;
	
	BinaryNode* root_node;

	std::stack<void*> unused;
	
	friend class BinaryNode;
};

class DiskNodeFileReadHandle : public NodeFileReadHandle {
public:
	DiskNodeFileReadHandle(std::string name);
	virtual ~DiskNodeFileReadHandle();

	virtual void close();
	virtual BinaryNode* getRootNode();

	virtual size_t size() {return file_size;}
	virtual size_t tell() {if(file) return ftell(file); return 0;}
protected:
	virtual bool renewCache();

	size_t file_size;
};

class MemoryNodeFileReadHandle : public NodeFileReadHandle {
public:
	MemoryNodeFileReadHandle(const uint8_t* data, size_t size);
	virtual ~MemoryNodeFileReadHandle();

	void assign(const uint8_t* data, size_t size);

	virtual void close();
	virtual BinaryNode* getRootNode();

	virtual size_t size() {return index - cache;}
	virtual size_t tell() {return cache_size;}
protected:
	virtual bool renewCache();

	uint8_t* index;
};

class FileWriteHandle : public FileHandle {
public:
	FileWriteHandle(std::string name);
	virtual ~FileWriteHandle();

	FORCEINLINE bool addU8(uint8_t u8) {return addType(u8);}
	FORCEINLINE bool addByte(uint8_t u8) {return addType(u8);}
	FORCEINLINE bool addU16(uint16_t u16) {return addType(u16);}
	FORCEINLINE bool addU32(uint32_t u32) {return addType(u32);}
	bool addString(const std::string& str);
	bool addString(const char* str);
	bool addLongString(const std::string& str);
	bool addRAW(const std::string& str);
	bool addRAW(const uint8_t* ptr, size_t sz);
	bool addRAW(const char* c) {return addRAW(reinterpret_cast<const uint8_t*>(c), strlen(c));}
protected:
	template<class T>
	bool addType(T ref) {
		fwrite(&ref, sizeof(ref), 1, file);
		return ferror(file) == 0;
	}
};

class NodeFileWriteHandle : public FileHandle {
public:
	NodeFileWriteHandle();
	virtual ~NodeFileWriteHandle();

	bool addNode(uint8_t nodetype);
	bool endNode();

	bool addU8(uint8_t u8);
	bool addByte(uint8_t u8);
	bool addU16(uint16_t u16);
	bool addU32(uint32_t u32);
	bool addString(const std::string& str);
	bool addLongString(const std::string& str);
	bool addRAW(std::string& str);
	bool addRAW(const uint8_t* ptr, size_t sz);
	bool addRAW(const char* c) {return addRAW(reinterpret_cast<const uint8_t*>(c), strlen(c));}

protected:
	virtual void renewCache() = 0;

	static uint8_t NODE_START;
	static uint8_t NODE_END;
	static uint8_t ESCAPE_CHAR;
	
	uint8_t* cache;
	size_t cache_size;
	size_t local_write_index;

	FORCEINLINE void writeBytes(const uint8_t* ptr, size_t sz) {
		if(sz) {
			do {
				if(*ptr == NODE_START || *ptr == NODE_END || *ptr == ESCAPE_CHAR) {
					cache[local_write_index++] = ESCAPE_CHAR;
					if(local_write_index >= cache_size) {
						renewCache();
					}
				}
				cache[local_write_index++] = *ptr;
				if(local_write_index >= cache_size) {
					renewCache();
				}
				++ptr;
				--sz;
			} while(sz != 0);
		}
	}
};

class DiskNodeFileWriteHandle : public NodeFileWriteHandle {
public:
	DiskNodeFileWriteHandle(std::string name);
	virtual ~DiskNodeFileWriteHandle();

	virtual void close();
protected:
	virtual void renewCache();
};

class MemoryNodeFileWriteHandle : public NodeFileWriteHandle {
public:
	MemoryNodeFileWriteHandle();
	virtual ~MemoryNodeFileWriteHandle();
	
	void reset();
	virtual void close();

	uint8_t* getMemory();
	size_t getSize();
protected:
	virtual void renewCache();
};

#endif
