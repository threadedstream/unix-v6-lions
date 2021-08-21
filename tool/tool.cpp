#include <iostream>
#include <string>
#include <fstream>
#include <cstdint>
#include <vector>
#include <filesystem>
#include <map>
#include <array>
#include <cstring>
#include <algorithm>

/*

*  Program Swapping Basic Input/Output Block Devices
    text.h 4300→
    text.c 4350→
    buf.h 4500→
    conf.h 4600→
    conf.c 4650→
    bio.c 4700→
    rk.c 5350→

    File and Directories File Systems Pipes
    file.h 5500→
    filsys.h 5550→
    ino.h 5600→
    inode.h 5650→
    sys2.c 5700→
    sys3.c 6000→
    rdwri.c 6200→
    subr.c 6400→
    fio.c 6600→
    alloc.c 6900→
    iget.c 7250→
    nami.c 7500→
    pipe.c 7700→


    Character Oriented Special Files
    tty.h 7900→
    kl.c 8000→
    tty.c 8100→
    pc.c 8600→
    lp.c 8800→
    mem.c 9000→

 */

#define INTERNAL_SERVER_ERROR 500

static const char *BASE_URL = "https://pages.lip6.fr/Pierre.Sens/srcv6/";

static std::array<std::filesystem::path, 5> directories = {
        "proc_html",
        "sys_html",
        "io_html",
        "filesys_html",
        "specf_html"
};


static std::map<std::filesystem::path, std::vector<const char *>> unixDirFilesMap = {
        {"proc_html",    {"low.s.html",   "m40.s.html",    "malloc.c.html",
                                 "prf.c.html",   "proc.h.html", "seg.h.html",
                                 "slp.c.html",   "user.h.html"}},
        {"sys_html",     {"clock.c.html", "reg.h.html",    "sig.c.html",
                                 "sys1.c.html",  "sys4.c.html", "sysent.c.html",
                                 "trap.c.html"}},
        {"io_html",      {"text.h.html",  "text.c.html",   "buf.h.html",
                                 "conf.h.html",  "conf.c.html", "bio.c.html",
                                 "rk.c.html"}},
        {"filesys_html", {"file.h.html",  "filsys.h.html", "ino.h.html",
                                 "inode.h.html", "sys2.c.html", "sys3.c.html",
                                 "rdwri.c.html", "subr.c.html", "fio.c.html",
                                 "alloc.c.html", "iget.c.html", "nami.c.html",
                                 "pipe.c.html"}},
        {"specf_html",   {"tty.h.html",   "kl.c.html",     "tty.c.html",
                                 "pc.c.html",    "lp.c.html",   "mem.c.html"}}
};

void parse(std::string &contents);

std::string parseFile(const char *path);

void parseDir(std::ifstream &stream, const std::filesystem::path &path);

void parseDirs();

void eliminateHtmlTags(std::string &contents, int &i);

void writeValidContentsToFile(std::string &contents, const char *fileName);

void downloadFiles();

void clean(const std::filesystem::path *problematicDir);


int main(int argc, char *argv[]) {
    //downloadFiles();
    parseDirs();
}

void parseDirs() {
    // allocation(1)
    std::ifstream stream;
    if (!stream) {
        clean(nullptr);
    }
    for (auto &dir : directories) {
        std::string desiredDir(std::move(dir.string()));
        auto idx = desiredDir.find_last_of('_');
        desiredDir = desiredDir.substr(0, idx);
        auto absolutePath = std::filesystem::absolute(dir);
        parseDir(stream, absolutePath);
    }
    stream.close();
}

void parseDir(std::ifstream &stream, const std::filesystem::path &dir) {
    std::vector<char> temp;
    std::string contents;

    auto dirIt = std::filesystem::directory_iterator(dir);
    for (auto const &entry : dirIt) {
        const auto absolutePath = std::filesystem::absolute(entry);
        stream.open(absolutePath.c_str());
        if (!stream.good()) {
            clean(nullptr);
        }

        auto absolutePathStr = absolutePath.string();
        const auto lastUnderscoreIdx = absolutePathStr.find_last_of('_');
        const auto lastSlashIdx = absolutePathStr.find_last_of('/');
        absolutePathStr = absolutePathStr.erase(lastUnderscoreIdx, lastSlashIdx - lastUnderscoreIdx);
        const auto lastDotIdx = absolutePathStr.find_last_of('.');
        absolutePathStr = absolutePathStr.substr(0, lastDotIdx);
        stream.seekg(0, std::ios::end);
        uint32_t size = stream.tellg();
        stream.seekg(0);
        temp.reserve(size);
        stream.read(temp.data(), size);
        if (!stream.good()) {
            clean(nullptr);
        }
        contents = temp.data();
        parse(contents);
        writeValidContentsToFile(contents, absolutePathStr.c_str());
        temp.clear();
        stream.close();
    }

}

void downloadFiles() {
    char command[256], outputFile[128];
    // Start an iteration
    for (const auto &dir : directories) {
        auto absolutePath = std::filesystem::absolute(dir);
        if (!std::filesystem::exists(absolutePath)) {
            bool created = std::filesystem::create_directory(absolutePath);
            if (!created) {
                clean(&absolutePath);
            }
        }
        for (const auto file : unixDirFilesMap[dir]) {
            sprintf(outputFile, "%s/%s", absolutePath.c_str(), file);
            sprintf(command, "wget %s%s -O %s", BASE_URL, file, outputFile);
            system(command);
        }
    }

}

// TODO(threadedstream): Add error report
void clean(const std::filesystem::path *problematicDir) {
    if (problematicDir == nullptr) {
        goto remove_no_problematic;
    }

    for (const auto &dir : directories) {
        if (dir == *problematicDir) {
            // We're done here, since we've reached a final, i.e problematic directory
            exit(INTERNAL_SERVER_ERROR);
        }
        // THOUGHTS(threadedstream): Not really sure how to handle errors
        std::filesystem::remove_all(dir);
    }

    remove_no_problematic:
    for (const auto &dir : directories) {
        std::filesystem::remove_all(dir);
    }

    exit(INTERNAL_SERVER_ERROR);
}

void eliminateHtmlTags(std::string &contents, int &i) {
    while (contents[i] != '>') {
        contents[i] = '\0';
        i++;
    }
    contents[i] = '\0';
}

void parse(std::string &contents) {
    int32_t i = 0;
    for (; i < contents.length() - 1;) {
        switch (contents[i]) {
            case '<':
                // start parsing tag
                eliminateHtmlTags(contents, i);
            default:
                i++;
        }
    }
}


void writeValidContentsToFile(std::string &contents, const char *path) {
    // WARNING(threadedstream): using 1024 as a heuristic value
    contents.erase(std::remove(contents.begin(), contents.end(), '\0'), contents.end());

    std::ofstream stream(path, std::fstream::out | std::fstream::app);
    if (!stream) {
        return;
    }


    stream.write(contents.c_str(), static_cast<long>(contents.size()));
    if (!stream) {
        return;
    }
    stream.close();
}

// The sole purpose of function below is to eliminate 4-digit numbers
// present in the beginning of almost each line in the C file.
// Got to figure out a way to do it effectively, error-free, and of course
// scalable
void eliminateBeginningNums(std::string &contents) {
    int32_t i = 0;
    for (; i < contents.length();) {
    }
}