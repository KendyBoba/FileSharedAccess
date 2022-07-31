#include "Buffer.h"

buffer::buffer(char* data, unsigned long long size) : this_data(data), size(size),length(size)
{

}
buffer::buffer() {

}

buffer::~buffer()
{
	delete[] this_data;
	this_data = nullptr;
}

void buffer::write(char* data, int size)
{
	int add = w_pointer;
	char* temp = data;
	for (int i = w_pointer; i < size + add; ++i) {
		if (i >= this->size) addSize();
		if (i >= length) ++length;
		this_data[i] = temp[i - add];
		++w_pointer;
	}
}

char* buffer::read(int size)
{
	int add = r_pointer;
	char* res = new char[size];
	for (int i = r_pointer; i < size + add; ++i) {
		res[i - add] = this_data[i];
		++r_pointer;
	}
	return res;
}

char* buffer::read(int size, int pos)
{
	r_pointer = pos;
	return this->read(size);
}

void buffer::cut(int pos, unsigned int size)
{
	for (int i = pos; i < this->length; ++i) {
		if (i + size >= this->length) this_data[i] = '\0';
		this_data[i] = this_data[i + size];
	}
	this->length -= size;
}

void buffer::write(char* data, int size, int pos)
{
	w_pointer = pos;
	this->write(data, size);
}

void buffer::addSize()
{
	const unsigned long long new_size = size + 0x100;
	char* new_data = new char[new_size];
	for (int i = 0; i < size; ++i) {
		new_data[i] = this_data[i];
	}
	delete[] this_data;
	this_data = new_data;
	new_data = nullptr;
	this->size = new_size;
}

void buffer::seekR(int pos)
{
	r_pointer = pos;
}

void buffer::seekW(int pos)
{
	w_pointer = pos;
}

int buffer::getPointR() const
{
	return r_pointer;
}

int buffer::getPointW() const
{
	return w_pointer;
}

char* buffer::data()
{
	return this_data;
}

unsigned long long buffer::getSize() const
{
	return this->size;
}

unsigned long long buffer::getLength() const
{
	return this->length;
}
