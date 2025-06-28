
#ifndef MAIN_FILE_H
#define MAIN_FILE_H

#include <stdio.h>
#include <iostream>
#include <cassert>
#include <set>

class FileDealer {
public:
	static FILE* fpInput;
	static FILE* fpOutput;
    static FILE* fpArm;
	static char* fileStr;
	
	static void inputDealer(const char *filename) {
		fpInput = fopen(filename, "r");
		assert(fpInput != nullptr);
		fseek(fpInput, SEEK_SET, SEEK_END);
		unsigned int fileSize = ftell(fpInput) + 1;
		rewind(fpInput);
		fileStr = new char[fileSize + 1](); // use () to initialize the memory
		fread(fileStr, 1, fileSize, fpInput);
        fileStr[fileSize] = 0;
        // printf("%s",fileStr);
	}

    static void outputOpen() {
        fpOutput = fopen("output.txt", "w");
#ifdef file_debug
        fpArm = fopen("mips.asm", "w");
#else
        fpArm = fopen("mips.txt", "w");
#endif
    }

    // static void LexerOutput(std::string str){
    //     fprintf(fpOutput,"%s",str.c_str());
    // }
    static void ParserOutput(const char *str) {
        // printf("%s\n", str.c_str(),flushall());
        // fprintf(fpOutput,"%s\n",str.c_str(),flushall());
        // printf("%s\n", str.c_str());
        fprintf(fpOutput, "%s\n", str);
    }

    static void fileClose() {
        fclose(fpInput);
        fclose(fpOutput);
        fclose(fpArm);
    }


};

#endif