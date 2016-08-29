#include <cstddef>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>

std::vector<std::string> split(const std::string &s, char delim, std::vector<std::string> &elems);

std::vector<std::string> split(const std::string &s, char delim);

std::vector<std::string> split2(const std::string &s, char delim, char delim2, std::vector<std::string> &elems);

std::vector<std::string> split2(const std::string &s, char delim, char delim2);

// trim from start
std::string &ltrim(std::string &s);

// trim from end
std::string &rtrim(std::string &s);

// trim from both ends
std::string &trim(std::string &s);

template <typename element_type>
void print_histogram(const element_type *values, const size_t len);
