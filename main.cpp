#include <iostream>
#include <vector>
#include <sstream>
#include "BlockDevice.hpp"

void help() {
    std::cout << "Commands:\n";
    std::cout << "  create <filename> <block_size> <block_count> - Crea un nuevo sistema de bloques\n";
    std::cout << "  open <filename> - Abre un bloque\n";
    std::cout << "  close - Cierra el bloque actualmente abierto\n";
    std::cout << "  write <block_number> <data> - Escribe data al bloque seleccionado\n";
    std::cout << "  read <block_number> <size> - Lee data desde el bloque seleccionado\n";
    std::cout << "  info - Muestra informacion actual del bloque\n";
    std::cout << "  exit - Termina el programa\n";

    std::cout << "\n-- ls - Lista los archivos\n";
    std::cout << "\n-- format - Formate el Device\n";
    std::cout << "\n-- wr <filename> <text> - Escribe texto en un archivo especificado \n";
    std::cout << "\n-- cat <filename> - Lee texto en el archivo especificado\n";
    std::cout << "\n-- hexdump <filename> - Lee texto de manera hexadecimal\n";
    std::cout << "\n-- copy_out <filename1> <filename2> - Copia FILENAME1 para pegar en el FILENAME2\n";
    std::cout << "\n-- copy_in <filename1> <filename2> - Copia FILENAME2 para pegar en el FILENAME1\\n";
    std::cout << "\n-- rm <filename> - Elimina el archivo\n";
    std::cout << "\n-- blocks - Imprime los bloques actuales\n";
}


int main() {
    BlockDevice device;
    std::string comando;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, comando);

        std::istringstream iss(comando);
        std::string cmd;
        iss >> cmd;

        if (cmd == "create") {
            std::string filename;
            std::size_t block_size, block_count;

            if (!(iss >> filename >> block_size >> block_count)) {
                std::cerr << "Error: Faltan Argumentos. Uso: create <filename> <block_size> <block_count>" << std::endl;
                continue;
            }

            if (block_size == 0 || block_count == 0) {
                std::cerr << "Error: BLOCK_SIZE o BLOCK_COUNT no valido." << std::endl;
                continue;
            }

            if (block_size < 137 || block_count < 137) {
                std::cerr << "Error: Un numero menor al tamaño del inodo ocasionara un crash." << std::endl;
                continue;
            }

            if (!device.create(filename, block_size, block_count)) {
                std::cerr << "Hubo un error al crear el device.\n";
            }

        } else if (cmd == "open") {
            std::string filename;

            if (!(iss >> filename)) {
                std::cerr << "Error: Faltan Argumentos. Uso: open <filename>" << std::endl;
                continue;
            }

            if(device.open(filename)) {
                std::cout << "Archivo abierto de manera exitosa." << std::endl;
            }

        } else if (cmd == "close") {
            if(device.close()) {
                std::cout << "Archivo cerrado de manera exitosa." << std::endl;
            }

        } else if (cmd == "write") {
            std::size_t block_number;
            std::string data;

            if (!(iss >> block_number)) {
                std::cerr << "Error: Faltan Argumentos. Uso: write <block_number> <data>" << std::endl;
                continue;
            }

            std::getline(iss, data);
            
            if (!data.empty() && data[0] == ' ') {
                data = data.substr(1);
            }

            if (data.empty()) {
                std::cerr << "Error: No se proporcionó datos para escribir." << std::endl;
                continue;
            }

            std::vector<char> dataVector(data.begin(), data.end());
            if (device.writeBlock(block_number, dataVector)) {
                std::cout << "Escrito de manera exitosa al bloque N° " << block_number << ".\n";
            }

        } else if (cmd == "read") {
            std::size_t block_number, primer_offset, ultimo_offset;

            if (!(iss >> block_number >> primer_offset >> ultimo_offset)) {
                std::cerr << "Error: Faltan Argumentos. Uso: read <block_number> <primer_offset> <ultimo_offset>" << std::endl;
                continue;
            }

            std::vector<char> data = device.readBlock(block_number);

            if (primer_offset >= ultimo_offset || ultimo_offset > data.size()) {
                std::cerr << "Error: Offset inválido o excede el tamaño del bloque." << std::endl;
                continue;
            }

            std::cout << std::hex;
            for (auto it = data.begin() + primer_offset; it != data.begin() + ultimo_offset; ++it) {
                std::cout << (int)(*it) << " ";
            }
            std::cout << std::dec << std::endl;

        } else if (cmd == "info") {
            device.info();
        } else if (cmd == "ls") {
            device.listFiles();
        } else if (cmd == "format") {
            if (device.format()) {
                std::cout << "Device formateado exitosamente.\n";
            } else {
                std::cerr << "Error al formatear el device.\n";
            }
        } else if (cmd == "wr") {
            std::string filename, text;

            if (!(iss >> filename)) {
                std::cerr << "Error: Faltan Argumentos. Uso: wr <filename> <text>" << std::endl;
                continue;
            }

            std::getline(iss, text);
            if (text.empty()) {
                std::cerr << "Error: No se proporcionó texto." << std::endl;
                continue;
            }

            if (device.write(filename, text)) {
                std::cout << "Texto escrito en el archivo " << filename << " exitosamente.\n";
            } else {
                std::cerr << "Error al escribir en el archivo.\n";
            }

        } else if (cmd == "cat") {
            std::string filename;
            if (!(iss >> filename)) {
                std::cerr << "Error: Faltan Argumentos. Uso: cat <filename>" << std::endl;
                continue;
            }

            device.cat(filename);
        } else if (cmd == "hexdump") {
            std::string filename;
            if (!(iss >> filename)) {
                std::cerr << "Error: Faltan Argumentos. Uso: hexdump <filename>" << std::endl;
                continue;
            }

            device.hexdump(filename);
        } else if (cmd == "copy_out") {
            std::string filename1, filename2;

            if (!(iss >> filename1 >> filename2)) {
                std::cerr << "Error: Faltan Argumentos. Uso: copy_out <filename1> <filename2>" << std::endl;
                continue;
            }

            if (device.copyOut(filename1, filename2)) {
                std::cout << "Archivo copiado a " << filename2 << " exitosamente.\n";
            } else {
                std::cerr << "Error al copiar el archivo.\n";
            }

        } else if (cmd == "copy_in") {
            std::string filename1, filename2;

            if (!(iss >> filename1 >> filename2)) {
                std::cerr << "Error: Faltan Argumentos. Uso: copy_in <filename1> <filename2>" << std::endl;
                continue;
            }

            if (device.copyIn(filename1, filename2)) {
                std::cout << "Archivo copiado a " << filename1 << " exitosamente.\n";
            } else {
                std::cerr << "Error al copiar el archivo.\n";
            }

        } else if (cmd == "rm") {
            std::string filename;
            if (!(iss >> filename)) {
                std::cerr << "Error: Faltan Argumentos. Uso: rm <filename>" << std::endl;
                continue;
            }

            if (device.remove(filename)) {
                std::cout << "Archivo " << filename << " eliminado exitosamente.\n";
            } else {
                std::cerr << "Error al eliminar el archivo.\n";
            }

        } else if (cmd == "help") {
            help();
        } else if (cmd == "exit") {
            std::cout << "Terminando programa...\n";
            break;
        } else {
            std::cerr << "Comando desconocido. Escribe 'help' para una lista de comandos disponibles.\n";
        }
    }
    return 0;
}