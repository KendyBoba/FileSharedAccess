#pragma once
class buffer {
private:
	char* this_data = nullptr;
	int r_pointer = 0;
	int w_pointer = 0;
	unsigned long long size = 0;
	unsigned long long length = 0;
private:
	void addSize();
public:
	buffer(char* data, unsigned long long size);
	buffer();
	~buffer();
	void write(char* data, int size);
	void write(char* data, int size, int pos);
	char* read(int size);
	char* read(int size,int pos);
	void cut(int pos,unsigned int size);
	void seekR(int pos);
	void seekW(int pos);
	int getPointR() const;
	int getPointW() const;
	char* data();
	unsigned long long getSize() const;
	unsigned long long getLength() const;
};