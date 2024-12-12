#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstdint>

struct Inodo
{
    char name[64];
    int64_t offset[8];
    size_t size;
    bool free;

    Inodo(const std::string &filename = "", bool _f = true) : free(_f)
    {
        std::strncpy(name, filename.c_str(), sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0'; // Asegurar terminación de cadena
        for (int i = 0; i < 8; i++)
        {
            offset[i] = -1;
        }
        size = 0;
    }
} __attribute__((packed));

struct Superblock
{
    uint64_t initialBlock;          // Bloque donde comienzan los datos
    uint64_t byteMapPos;            // Bloque donde comienza el mapa de bloques libres
    uint64_t inodesInitialBlockPos; // Bloque donde comienzan los inodos
    uint64_t inodesPerBlock;        // Cantidad de inodos por bloque

    Superblock()
        : initialBlock(0),
          byteMapPos(0),
          inodesInitialBlockPos(0),
          inodesPerBlock(0) {}

    Superblock(uint64_t _inodesPerBlock, uint64_t _inodesInitialBlockPos)
        : byteMapPos(1),
          inodesPerBlock(_inodesPerBlock),
          inodesInitialBlockPos(_inodesInitialBlockPos),
          initialBlock(_inodesInitialBlockPos + 1) {}
};

class BlockDevice
{
private:
    std::fstream file;
    Superblock superblock;
    std::vector<bool> freeBlockMap;
    size_t blockSize;

    size_t getBlockCount();
    size_t getBlockSize() { return blockSize; };
    int getEstado(size_t index);

    int64_t buscarInodo(const std::string &filename);
    Inodo readInode(int64_t offset);
    void writeInode(int64_t offset, const Inodo &inode);

    size_t buscarInodoLibre();
    size_t getSizeMapBlocks();
    void initializeSuperblock(size_t blockSize, size_t blockCount);


public:
    bool create(const std::string &filename, size_t blockSize, size_t blockCount);
    bool open(const std::string &filename);
    bool close();
    bool writeBlock(size_t blockNumber, const std::vector<char> &data);
    std::vector<char> readBlock(size_t blockNumber);
    std::vector<char> read(const std::string &filename);
    void info();

    bool format();
    void listFiles();
    void cat(const std::string &file);
    void hexdump(const std::string &file);
    bool write(const std::string &file, const std::string &text);
    bool copyOut(const std::string &file1, const std::string &file2);
    bool copyIn(const std::string &file1, const std::string &file2);
    bool remove(const std::string &file);
};