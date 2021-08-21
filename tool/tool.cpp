#include <iostream>
#include <string>
#include <cassert>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <map>

enum {
    TOKEN_A_OPEN_TAG,
    TOKEN_A_CLOSE_TAG,
    TOKEN_HTML_OPEN_TAG,
    TOKEN_HEAD_OPEN_TAG,
    TOKEN_META_OPEN_TAG,
};

static std::map<std::string, int> tokens = {
        {"<a>",  TOKEN_A_OPEN_TAG},
        {"</a>", TOKEN_A_CLOSE_TAG}
};

void parseTag(std::string &contents, int &i);

void writeValidContentsToFile(const std::string &contents, const char* fileName);

int main(int argc, char *argv[]) {
    assert(argc == 2 && "expected at least a single file");
    std::string contents;
    char *temp;
    std::ifstream stream(argv[1]);
    if (!stream) {
        return -1;
    }
    auto fileStr = std::string(argv[1]);
    const auto lastDotIdx = fileStr.find_last_of('.');
    const auto cFileName = fileStr.substr(0, lastDotIdx);
    stream.seekg(0, std::ios::end);
    uint32_t size = stream.tellg();
    stream.seekg(0);
    temp = new char[size + 1];
    stream.read(temp, size);
    if (!stream) {
        std::cerr << "failed to read contents of a file";
        free(temp);
        exit(-1);
    }
    stream.close();
    contents = temp;
    delete temp;
    int32_t i = 0;
    for (; i < contents.length() - 1;) {
        switch (contents[i]) {
            case '<':
                // start parsing tag
                parseTag(contents, i);
            default:
                i++;
        }
    }

    writeValidContentsToFile(contents, cFileName.c_str());
}

void parseTag(std::string &contents, int &i) {
    while (contents[i] != '>') {
        contents[i] = '\0';
        i++;
    }
    contents[i] = '\0';
}

void writeValidContentsToFile(const std::string &contents, const char* fileName) {
    std::string newContents;
    for (const auto c : contents) {
        if (c != '\0') {
            newContents += c;
        }
    }
    std::cout << newContents;
    std::ofstream stream(fileName, std::fstream::out | std::fstream::app);
    if (!stream){
        return;
    }

    stream.write(newContents.c_str(), static_cast<long>(newContents.size()));
    if (!stream) {
        return;
    }
    stream.close();
}

void eliminateBeginningNums(std::string& contents) {
    int32_t i = 0;
    for (; i < contents.length();) {
        w
    }
}