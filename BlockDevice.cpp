#include "BlockDevice.hpp"


int64_t BlockDevice::buscarInodo(const std::string &filename) {
    for (size_t block = superblock.inodesInitialBlockPos; block < superblock.inodesInitialBlockPos + getBlockCount(); block++) {
        std::vector<char> rawData = readBlock(block);
        if (rawData.empty()) continue;

        size_t inodesPerBlock = getBlockSize() / sizeof(Inodo);

        for (size_t i = 0; i < inodesPerBlock; i++) {
            Inodo inode;
            std::memcpy(&inode, &rawData[i * sizeof(Inodo)], sizeof(Inodo));

            if (!inode.free && filename == inode.name) {
                return block * getBlockSize() + i * sizeof(Inodo);
            }
        }
    }

    return -1;
}

Inodo BlockDevice::readInode(int64_t offset) {
    Inodo inode;
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char *>(&inode), sizeof(Inodo));
    return inode;
}


void BlockDevice::writeInode(int64_t offset, const Inodo &inode) {
    file.seekp(offset, std::ios::beg);
    file.write(reinterpret_cast<const char *>(&inode), sizeof(Inodo));
}

size_t BlockDevice::buscarInodoLibre() {
    size_t inodesPerBlock = getBlockSize() / sizeof(Inodo);
    for (size_t block = superblock.inodesInitialBlockPos; block < getBlockCount(); ++block) {
        std::vector<Inodo> inodes(inodesPerBlock);
        auto rawData = readBlock(block);

        std::memcpy(inodes.data(), rawData.data(), rawData.size());

        for (size_t i = 0; i < inodesPerBlock; ++i) {
            if (inodes[i].free) return block * getBlockSize() + i * sizeof(Inodo);
        }
    }
    return -1;
}

void BlockDevice::initializeSuperblock(size_t blockSize, size_t blockCount)
{
    superblock.initialBlock = blockCount;
    superblock.byteMapPos = 1;
    superblock.inodesInitialBlockPos = 2;
    superblock.inodesPerBlock = blockSize / sizeof(Inodo);

    this->blockSize = blockSize;
}

bool BlockDevice::create(const std::string &filename, size_t blockSize, size_t blockCount) {
    file.open(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    initializeSuperblock(blockSize, blockCount);

    file.write(reinterpret_cast<const char *>(&superblock), sizeof(Superblock));

    freeBlockMap.resize(blockCount, true);

    std::vector<char> emptyBlock(blockSize, 0);
    for (size_t i = 0; i < blockCount; i++) {
        file.write(emptyBlock.data(), emptyBlock.size());
    }

    file.close();
    return true;
}

bool BlockDevice::open(const std::string &filename) {
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(&superblock), sizeof(Superblock));

    freeBlockMap.resize(getBlockCount(), true);

    return true;
}

bool BlockDevice::close() {
    if (file.is_open()) {
        file.close();
        return true;
    }
    return false;
}

bool BlockDevice::writeBlock(size_t blockNumber, const std::vector<char> &data) {
    if (blockNumber >= getBlockCount() || data.size() > getBlockSize()) return false;

    file.seekp(blockNumber * getBlockSize(), std::ios::beg);
    file.write(data.data(), data.size());

    freeBlockMap[blockNumber] = false;
    return true;
}

std::vector<char> BlockDevice::read(const std::string &filename)
{
    if (!file.is_open()) {
        std::cerr << "Error: File not open.\n";
        return std::vector<char>();
    }

    int64_t offset = buscarInodo(filename);
    if (offset == -1) {
        std::cerr << "Error: No inode found for file " << filename << ".\n";
        return std::vector<char>();
    }

    file.flush();
    std::vector<char> text;

    Inodo inode;
    file.seekg(offset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&inode), sizeof(Inodo));

    if (file.fail()) {
        std::cerr << "Error: Failed to read inode at offset " << offset << ".\n";
        return std::vector<char>();
    }

    int64_t size = inode.size;
    size_t block = getBlockSize();
    int i = 0;

    while (size > 0) {
        std::vector<char> line;
        if (size > block) {
            line = std::vector<char>(block);
            file.seekg(inode.offset[i] * block, std::ios::beg);  
            file.read(line.data(), block);
            size -= block;
        } else {
            line = std::vector<char>(size);
            file.seekg(inode.offset[i] * block, std::ios::beg);
            file.read(line.data(), size);
            size = 0;
        }

        if (file.fail()) {
            std::cerr << "Error: Failed to read block data.\n";
            return std::vector<char>();
        }

        text.insert(text.end(), line.begin(), line.end());
        i++;
    }

    return text;
}

std::vector<char> BlockDevice::readBlock(size_t blockNumber) {
    std::vector<char> data(getBlockSize());
    if (blockNumber >= getBlockCount()) return {};

    file.seekg(blockNumber * getBlockSize(), std::ios::beg);
    file.read(data.data(), data.size());

    return data;
}

void BlockDevice::info() {
    if (file.is_open()) {
        std::cout << "Info:\n";
        std::cout << "  Initial Block: " << superblock.initialBlock << "\n";
        std::cout << "  Inodes Per Block: " << superblock.inodesPerBlock << "\n";
    } else {
        std::cerr << "Error: No block device is open.\n";
    }
}

bool BlockDevice::format() {
    if (!file.is_open()) {
        std::cerr << "Error: No block device is open.\n";
        return false;
    }

    initializeSuperblock(getBlockSize(), getBlockCount());

    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<const char *>(&superblock), sizeof(Superblock));

    freeBlockMap.assign(getBlockCount(), true);

    std::vector<char> emptyBlock(getBlockSize(), 0);

    for (size_t i = 0; i < getBlockCount(); i++) {
        file.seekp(i * getBlockSize()); 
        file.write(emptyBlock.data(), emptyBlock.size()); 
    }

    size_t inodesPerBlock = getBlockSize() / sizeof(Inodo);
    size_t inodeBlockCount = (getBlockCount() + inodesPerBlock - 1) / inodesPerBlock;


    for (size_t block = superblock.inodesInitialBlockPos; block < getBlockCount(); ++block) {
        std::vector<Inodo> inodes(inodesPerBlock);

        for (size_t i = 0; i < inodesPerBlock; ++i) {
            inodes[i].free = true;
            inodes[i].name[0] = '\0';
            inodes[i].size = 0;
        }

        file.seekp(block * getBlockSize());
        file.write(reinterpret_cast<const char *>(inodes.data()), inodes.size() * sizeof(Inodo));
    }

    return true;
}


void BlockDevice::listFiles()
{
    if (!file.is_open()) {
        std::cerr << "The file is not open.\n";
        return;
    }

    size_t block_size = getBlockSize();
    bool foundFile = false;

    for (size_t block = superblock.inodesInitialBlockPos; block < getBlockCount(); ++block) {
        std::vector<char> rawData = readBlock(block);

        size_t inodesPerBlock = block_size / sizeof(Inodo);

        for (size_t i = 0; i < inodesPerBlock; ++i) {
            Inodo inode;
            std::memcpy(&inode, &rawData[i * sizeof(Inodo)], sizeof(Inodo));

            if (!inode.free && inode.name[0] != '\0' && inode.size > 0) {
                if (!foundFile) {
                    std::cout << "Archivos disponibles en el sistema:\n";
                    foundFile = true;
                }
                std::cout << "- Archivo: " << inode.name << " (Tamaño: " << inode.size << " bytes)\n";
            }
        }
    }

    if (!foundFile) {
        std::cout << "Ningun archivo disponible.\n";
    }
}


void BlockDevice::cat(const std::string &file)
{
    int64_t inodeOffset = buscarInodo(file);
    if (inodeOffset == -1)
    {
        std::cerr << "File not found.\n";
        return;
    }

    Inodo inode = readInode(inodeOffset);
    size_t blockCount = inode.size / getBlockSize() + 1;
    for (size_t i = 0; i < blockCount; ++i)
    {
        auto data = readBlock(inode.offset[i]);
        std::cout.write(data.data(), data.size());
    }
}

void BlockDevice::hexdump(const std::string &file) {
    std::vector<char> text = read(file);
    if (text.empty()) {
        std::cout << "No contiene texto.\n";
        return;
    }

    for (size_t i = 0; i < text.size(); i++) {
        unsigned int num = static_cast<unsigned char>(text[i]);
        std::cout << std::hex << std::setw(2) << std::setfill('0') << num << " ";
        if ((i + 1) % 16 == 0) {
            std::cout << std::endl;
        }
    }

    std::cout << std::dec << "\n\n";
}

bool BlockDevice::write(const std::string &file, const std::string &text)
{
    int64_t inodeOffset = buscarInodo(file);
    if (inodeOffset == -1)
    {
        size_t freeInodeOffset = buscarInodoLibre();
        if (freeInodeOffset == -1)
        {
            std::cerr << "NO hay ninguna inodo disponible.\n";
            return false;
        }
        
        Inodo newInode;
        std::strncpy(newInode.name, file.c_str(), sizeof(newInode.name) - 1);
        newInode.size = text.size();
        newInode.free = false;
        writeInode(freeInodeOffset, newInode);
        inodeOffset = freeInodeOffset;
    }

    Inodo inode = readInode(inodeOffset);
    size_t blockCount = (inode.size + getBlockSize() - 1) / getBlockSize();
    for (size_t i = 0; i < blockCount; ++i)
    {
        std::vector<char> data(getBlockSize(), 0);
        size_t chunkSize = std::min(inode.size - i * getBlockSize(), getBlockSize());
        std::memcpy(data.data(), text.data() + i * getBlockSize(), chunkSize);
        writeBlock(inode.offset[i], data);
    }

    return true;
}

bool BlockDevice::copyOut(const std::string &file1, const std::string &file2)
{
    int64_t inodeOffset = buscarInodo(file1);
    if (inodeOffset == -1)
    {
        std::cerr << "File not found.\n";
        return false;
    }

    Inodo inode = readInode(inodeOffset);
    std::ofstream out(file2, std::ios::binary);
    if (!out.is_open())
    {
        return false;
    }

    size_t blockCount = inode.size / getBlockSize() + 1;
    for (size_t i = 0; i < blockCount; ++i)
    {
        auto data = readBlock(inode.offset[i]);
        out.write(data.data(), data.size());
    }

    return true;
}

bool BlockDevice::copyIn(const std::string &file1, const std::string &file2)
{
    std::ifstream in(file1, std::ios::binary);
    if (!in.is_open())
    {
        return false;
    }

    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return write(file2, text);
}

bool BlockDevice::remove(const std::string &file)
{
    int64_t inodeOffset = buscarInodo(file);
    
    if (inodeOffset == -1)
    {
        std::cerr << "Archivo no encontrado.\n";
        return false;
    }

    Inodo inode = readInode(inodeOffset);

    inode.free = true;

    writeInode(inodeOffset, inode);

    return true;
}

size_t BlockDevice::getBlockCount() {
    file.seekg(0, std::ios::end);
    return file.tellg() / getBlockSize();
}

int BlockDevice::getEstado(size_t index) {
    if (index >= freeBlockMap.size()) return -1;
    return freeBlockMap[index];
}

