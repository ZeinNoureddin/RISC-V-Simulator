#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <utility>
#include <iomanip>

using namespace std; 

//the program will terminate at any instance where the flag is set to true
bool flag = false;

vector<string> riscv_registers = {
    "zero", "ra", "sp", "gp", "tp",
    "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6"
}; 

//assumption: x0 and x1 will not be acceptable inputs, instead use zero and ra.

////////////////////////////////////////////////////////////////
// FUNCTIONS WE USE THROUGHOUT THE PROGRAM
////////////////////////////////////////////////////////////////

bool has_non_space_characters_after_colon(string s) {
    // Find the position of the first colon in the string
    size_t colon_pos = s.find(":");
    if (colon_pos != string::npos) {
        // Check if there are any non-space characters after the colon
        size_t non_space_pos = s.find_first_not_of(" \t", colon_pos+1);
        if (non_space_pos != string::npos) {
            return true;
        }
    }
    return false;
}

pair<string, int> getOffset(string toParse){
    string regName;
    int offset; 
    size_t openParen = toParse.find('(');
    size_t closeParen = toParse.find(')');
    offset = stoi(toParse.substr(0, openParen));
    regName = toParse.substr(openParen + 1, closeParen - openParen - 1);
    return make_pair(regName, offset);
}

string dec2Hexadecimal(int decimal){
		stringstream ss;
		ss << hex << decimal;
		string hexa(ss.str());
		return hexa;
}

int signedBinaryToDecimal(string binaryString) {
    int decimal = 0;
    int power = 0;
    bool isNegative = false;

    if (binaryString[0] == '1') {
        isNegative = true;
        // Invert all bits
        for (int i = 0; i < binaryString.length(); i++) {
            if (binaryString[i] == '0') {
                binaryString[i] = '1';
            } else {
                binaryString[i] = '0';
            }
        }
        // Add 1 to get the 2's complement
        for (int i = binaryString.length() - 1; i >= 0; i--) {
            if (binaryString[i] == '0') {
                binaryString[i] = '1';
                break;
            } else {
                binaryString[i] = '0';
            }
        }
    }

    for (int i = binaryString.length() - 1; i >= 0; i--) {
        if (binaryString[i] == '1') {
            decimal += pow(2, power);
        }
        power++;
    }

    if (isNegative) {
        decimal = -decimal;
    }

    return decimal;
}

string decToBin(int n)
	{
		string bitnum = "";
		for (int i = 31; i >= 0; i--)
		{
			int x = n >> i;
			if (x & 1)
				bitnum.push_back('1');
			else
				bitnum.push_back('0');
		}
		return bitnum;
	}

int binaryToDecimal(string binaryString) {
    int decimal = 0;
    int power = 0;
    for (int i = binaryString.length() - 1; i >= 0; i--) {
        if (binaryString[i] == '1') {
            decimal += pow(2, power);
        }
        power++;
    }
    return decimal;
}

string zeroPadding(string binary){
    string padded = "";
    for(int i = 0; i < 32 - binary.length(); i++){
        padded += "0";
    }
    padded += binary;
    return padded;
}


string onePadding(string binary){
    string padded = "";
    for(int i = 0; i < 32 - binary.length(); i++){
        padded += "1";
    }
    padded += binary;
    return padded;
}

////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
////////////////////////////////////////////////////////////////

map<string, int> registers; //gloabal map that maps the register names to their values
map<int, string> mem; //gloabal map that maps the memory addresses to their values
map<string, int> labels; //global map that maps the labels to their line numbers

////////////////////////////////////////////////////////////////
// MORE FUNCTIONS THAT WE USE THROUGHOUT THE PROGRAM
////////////////////////////////////////////////////////////////

void readData(map<int, string>& mem, string filename) {
    ifstream dataFile(filename);
    if (!dataFile) {
        cerr << "Error: could not open file" << endl;
        return;
    }
    string line;
     while (getline(dataFile, line)) {
        stringstream ss(line);
        string num1Str, num2Str;
        getline(ss, num1Str, ':');
        getline(ss, num2Str);
        int num1 = stoi(num1Str);

        int num2 = stoi(num2Str);

        string num2Padded = zeroPadding(decToBin(num2));

        string firstByte = num2Padded.substr(0, 8);
        string secondByte = num2Padded.substr(8, 8);
        string thirdByte = num2Padded.substr(16, 8);
        string fourthByte = num2Padded.substr(24, 8);


        mem[num1] = fourthByte;
        mem[num1 + 1] = thirdByte;
        mem[num1 + 2] = secondByte;
        mem[num1 + 3] = firstByte;

    }
    //print memory
    for (auto it = mem.begin(); it != mem.end(); it++) {
        cout << it->first << ": " << it->second << endl;
    }

    dataFile.close();
}


//makes sure that the register names are valid and that the rd register is not zero
void validate_3input_registers(string instruction, string rd, string rs1, string rs2){
    bool rd_present = find(riscv_registers.begin(), riscv_registers.end(), rd) != riscv_registers.end();
    bool rs1_present = find(riscv_registers.begin(), riscv_registers.end(), rs1) != riscv_registers.end();
    bool rs2_present = find(riscv_registers.begin(), riscv_registers.end(), rs2) != riscv_registers.end();

    if (!rd_present || !rs1_present || !rs2_present) {
        cout << "ERROR: Invalid register name" << endl;
        flag = true;
        return;
    }

    if (rd == "zero") {
        cout << "ERROR: Cannot write to zero register" << endl;
        flag = true;
        return;
    }
}

void validate_2input_registers(string instruction, string rd, string rs1){
    bool rd_present = find(riscv_registers.begin(), riscv_registers.end(), rd) != riscv_registers.end();
    bool rs1_present = find(riscv_registers.begin(), riscv_registers.end(), rs1) != riscv_registers.end();

    if (!rd_present || !rs1_present) {
        cout << "ERROR: Invalid register name" << endl;
        flag = true;
        return;
    }

    if (rd == "zero" && instruction != "jalr" && instruction != "beq" && instruction != "bne" && instruction != "blt" && instruction != "bge" && instruction != "bltu" && instruction != "bgeu") {
        cout << "ERROR: Cannot write to zero register" << endl;
        flag = true;
        return;
    }
}

void validate_1input_registers(string rd){
    bool rd_present = find(riscv_registers.begin(), riscv_registers.end(), rd) != riscv_registers.end();

    if (!rd_present) {
        cout << "ERROR: Invalid register name" << endl;
        flag = true;
        return;
    }

    if (rd == "zero") {
        cout << "ERROR: Cannot write to zero register" << endl;
        flag = true;
        return;
    }
}

////////////////////////////////////////////////////////////////
// INSTRUCTION FUNCTIONS
////////////////////////////////////////////////////////////////

// R-TYPE INSTRUCTIONS
void add(string rd, string rs1, string rs2){
    validate_3input_registers("add", rd, rs1, rs2);
    registers[rd] = registers[rs1] + registers[rs2];
}
void sub(string rd, string rs1, string rs2){
    validate_3input_registers("sub", rd, rs1, rs2);
    registers[rd] = registers[rs1] - registers[rs2];
}
void sll(string rd, string rs1, string rs2){
    validate_3input_registers("sll", rd, rs1, rs2);
    registers[rd] = registers[rs1] << registers[rs2];
}
void slt(string rd, string rs1, string rs2){
    validate_3input_registers("slt", rd, rs1, rs2);
    registers[rd] = registers[rs1] < registers[rs2];
}
// COME BACK TO THIS ONE // rege3na walla lessa?
void sltu(string rd, string rs1, string rs2){
    validate_3input_registers("sltu", rd, rs1, rs2);
    unsigned int x = registers[rs1];
    unsigned int y = registers[rs2];
    registers[rd] = x < y;
}
void XOR(string rd, string rs1, string rs2){
    validate_3input_registers("XOR", rd, rs1, rs2);
    registers[rd] = registers[rs1] ^ registers[rs2];
}
void srl(string rd, string rs1, string rs2){
    validate_3input_registers("srl", rd, rs1, rs2);
    if(registers[rs1] < 0){
        unsigned int x = registers[rs1];
        registers[rd] = x >> registers[rs2];
    }
    else
        registers[rd] = (registers[rs1] >> registers[rs2]) + pow(2, 31);
}
void sra(string rd, string rs1, string rs2){
    validate_3input_registers("sra", rd, rs1, rs2);
    registers[rd] = registers[rs1] >> registers[rs2];
}
void OR(string rd, string rs1, string rs2){
    validate_3input_registers("OR", rd, rs1, rs2);
    registers[rd] = registers[rs1] | registers[rs2];
}
void AND(string rd, string rs1, string rs2){
    validate_3input_registers("AND", rd, rs1, rs2);
    registers[rd] = registers[rs1] & registers[rs2];
}


// -TYPE INSTRUCTIONS
void slli(string rd, string rs1, int imm){
    //check if immediate value is out of range
    if(imm > 31 || imm < 0){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }

    validate_2input_registers("slli", rd, rs1);

    registers[rd] = registers[rs1] << imm;
}

void srli(string rd, string rs1, int imm){
    if(imm > 31 || imm < 0){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }

    validate_2input_registers("srli", rd, rs1);

    if(registers[rs1] < 0){
        unsigned int x = registers[rs1];
        registers[rd] = x >> imm;
    }
    else
        registers[rd] = (registers[rs1] >> imm) + pow(2, 31);
}

void srai(string rd, string rs1, int imm){
    if(imm > 31 || imm < 0){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("srai", rd, rs1);
    registers[rd] = registers[rs1] >> imm;
}

// I-TYPE INSTRUCTIONS
void ADDI(string rd, string rs1, int imm){
    if(imm > 2047 || imm < -2048){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("ADDI", rd, rs1);
    registers[rd] = registers[rs1] + imm;
}
void SLTI(string rd, string rs1, int imm){
    if(imm > 2047 || imm < -2048){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("SLTI", rd, rs1);
    registers[rd] = registers[rs1] < imm;
}
void SLTIU(string rd, string rs1, int imm){
    if(imm > 2047 || imm < -2048){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("SLTIU", rd, rs1);
    unsigned int x = registers[rs1];
    registers[rd] = x < imm;
}
void XORI(string rd, string rs1, int imm){
    if(imm > 2047 || imm < -2048){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("XORI", rd, rs1);
    registers[rd] = registers[rs1] ^ imm;
}
void ORI(string rd, string rs1, int imm){
    if(imm > 2047 || imm < -2048){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("ORI", rd, rs1);
    registers[rd] = registers[rs1] | imm;
}
void ANDI(string rd, string rs1, int imm){
    if(imm > 2047 || imm < -2048){
        cout << "ERROR: IMMEDIATE VALUE OUT OF RANGE" << endl;
        flag = true;
        return;
    }
    validate_2input_registers("ANDI", rd, rs1);
    registers[rd] = registers[rs1] & imm;
}

void LW(string rd, int offset, string rs1){
    string firstByte = mem[offset + registers[rs1]];
    string secondByte = mem[offset + registers[rs1] + 1];
    string thirdByte = mem[offset + registers[rs1] + 2];
    string fourthByte = mem[offset + registers[rs1] + 3];
    string word = fourthByte + thirdByte + secondByte + firstByte;
    registers[rd] = signedBinaryToDecimal(word);    
}

void LH(string rd, int offset, string rs1){
    string firstByte = mem[offset + registers[rs1]];
    string secondByte = mem[offset + registers[rs1] + 1];
    string halfword = secondByte + firstByte;

    registers[rd] = signedBinaryToDecimal(halfword);

}

void LHU(string rd, int offset, string rs1){
    string firstByte = mem[offset + registers[rs1]];
    string secondByte = mem[offset + registers[rs1] + 1];
    string halfword = secondByte + firstByte;
    registers[rd] = binaryToDecimal(halfword);
}

void LB(string rd, int offset, string rs1){
    string firstByte = mem[offset + registers[rs1]];
    registers[rd] = signedBinaryToDecimal(firstByte);
}

void LBU(string rd, int offset, string rs1){
    string firstByte = mem[offset + registers[rs1]];
    registers[rd] = binaryToDecimal(firstByte);
}

//S-TYPE INSTRUCTIONS
void SW(string rs2, int offset, string rs1){

    int num = registers[rs2];
    string padded;

    if(num < 0){
        string numBin = decToBin(registers[rs2]);
        padded = onePadding(numBin);
    }
    else{
        string numBin = decToBin(registers[rs2]);
        padded = zeroPadding(numBin);
    }

    string firstByte = padded.substr(0, 8);
    string secondByte = padded.substr(8, 8);
    string thirdByte = padded.substr(16, 8);
    string fourthByte = padded.substr(24, 8);

    mem[offset + registers[rs1]] =  fourthByte;
    mem[offset + registers[rs1] + 1] =  thirdByte;
    mem[offset + registers[rs1] + 2] = secondByte;
    mem[offset + registers[rs1] + 3] = firstByte;

    
}

void SH(string rs2, int offset, string rs1){
   
    int num = registers[rs2];
    string padded;

    if(num < 0){
        string numBin = decToBin(registers[rs2]);
        padded = onePadding(numBin);
    }
    else{
        string numBin = decToBin(registers[rs2]);
        padded = zeroPadding(numBin);
    }

    string firstByte = padded.substr(0, 8);
    string secondByte = padded.substr(8, 8);

    mem[offset + registers[rs1]] =  secondByte;
    mem[offset + registers[rs1] + 1] =  firstByte;

}

void SB(string rs2, int offset, string rs1){

    int num = registers[rs2];
    string padded;

    if(num < 0){
        string numBin = decToBin(registers[rs2]);
        padded = onePadding(numBin);
    }
    else{
        string numBin = decToBin(registers[rs2]);
        padded = zeroPadding(numBin);
    }

    string firstByte = padded.substr(0, 8);

    mem[offset + registers[rs1]] = firstByte;

}   

// B-TYPE INSTRUCTION 
void beq(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("beq", rs1, rs2);
    if(registers[rs1] == registers[rs2]){
        i = imm; 
    }
}

void bne(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("bne", rs1, rs2);
    if(registers[rs1] != registers[rs2]){
        i = imm; 
    }
}

void blt(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("blt", rs1, rs2);
    if(registers[rs1] < registers[rs2]){
        i = imm; 
    }
}

void ble(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("blt", rs1, rs2);
    if(registers[rs1] <= registers[rs2]){
        i = imm; 
    }
}

void bgt(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("bgt", rs1, rs2);
    if(registers[rs1] > registers[rs2]){
        i = imm; 
    }
}

void bge(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("bge", rs1, rs2);
    if(registers[rs1] >= registers[rs2]){
        i = imm; 
    }
}

void bgeu(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("bgeu", rs1, rs2);
    unsigned int r1, r2;
    r1 = registers[rs1];
    r2 = registers[rs2];
    if(r1 >= r2){
        i = imm; 
    }
}

void bltu(string rs1, string rs2, int imm, int& i){
    validate_2input_registers("bltu", rs1, rs2);
    unsigned int r1, r2;
    r1 = registers[rs1];
    r2 = registers[rs2];
    if(r1 < r2){
        i = imm; 
    }
}

// J-TYPE INSTRUCTIONS

void jal(string rd, int imm, int& i){

    validate_1input_registers(rd);

    if (rd != "zero"){
        registers[rd] = i + 1;
    }

    i = imm;

}

void jalr(string rd, int offset, string rs1, int& i){
    validate_2input_registers("jalr", rd, rs1);
    if (rd != "zero"){
        registers[rd] = i + 1;
    }
    i = registers[rs1] + offset;
}

// U-TYPE INSTRUCTIONS
void lui(string rd, int imm){
    validate_1input_registers(rd);
    registers[rd] = imm << 12;
}

void auipc(string rd, int imm){   
    validate_1input_registers(rd);
    registers[rd] = registers[rd] + (imm << 12);
} 

void ecall(){
    exit(0);
}

void ebreak(){
    exit(0);
}

void fence(){
    exit(0);
}

//function that parses each instruction into its components
void parseInputString_3operands(string input, string& instruction, string& rd, string& rs1, string& rs2) {
    size_t pos1 = input.find(',');
    size_t pos2 = input.find(',', pos1 + 1);

    instruction = input.substr(0, input.find(' '));
    rd = input.substr(input.find(' ') + 1, pos1 - input.find(' ') - 1);
    rs1 = input.substr(pos1 + 2, pos2 - pos1 - 2);
    rs2 = input.substr(pos2 + 2);
}

void parseInputString_2operands(const string& input, string& instruction, string& rd, string& rs) {
    size_t pos1 = input.find(' ');
    size_t pos2 = input.find(',', pos1 + 1);
    instruction = input.substr(0, pos1);
    rd = input.substr(pos1 + 1, pos2 - pos1 - 1);
    rs = input.substr(pos2 + 2);

}


// function that puts the instructions into a vector
void putIntoVector(string FileName, vector<string>& commands){
//opens the file and makes sure it is opened correctly
    ifstream inputStream; 
    inputStream.open(FileName);
    if(inputStream.fail()){
		cout << "Problem in opening the file\n";
		exit(-1);
	}

    //reads the file line by line and stores each line in a vector
    //possible limitation: it will store empty lines as well
    string command;
    while(getline(inputStream, command)){
        if (command != "") {
            commands.push_back(command);
        }
    }

    // loop that checks if an instruction has a label preceding it and if it does it puts it in a separate string in the vector at the index before the instruction
    for (int i = 0; i < commands.size(); i++) {
        // Check if the current instruction starts with a label
        size_t colon_pos = commands[i].find(":");
        bool isThereCharStill = has_non_space_characters_after_colon(commands[i]);
        if(!isThereCharStill){
            continue;
        }
        if (colon_pos != string::npos) {
            // Extract the label and remove it from the instruction
            string label = commands[i].substr(0, colon_pos+1);
            commands[i].erase(0, colon_pos+1);

            if(commands[i][0] == ' '){
                commands[i].erase(0, 1);
            }

            // Insert the label in the position previous to the current index
            if (i > 0) {
                commands.insert(commands.begin()+i, label);
                i++;
            } else {
                commands.insert(commands.begin(), label);
            }
        }
    }

    // loop that checks if an instruction jumps to a label, it gets the index of the label and stores it in a map
    for (int i = 0; i < commands.size(); i++) {
        // Check if the current instruction is a label
        size_t colon_pos = commands[i].find(":");
        if (colon_pos != string::npos) {
            // Extract the label and remove it from the instruction
            string labelName = commands[i].substr(0, colon_pos);
            //labels.insert(pair<string, int>(label, i));
            labels[labelName] = i;
            // cout << labelName << "OMARADS" << i << endl;
        }
    }
    // print the labels map
    // for (auto it = labels.begin(); it != labels.end(); it++) {
    //     cout << it->first << " " << it->second << endl;
    // }
    // TESTING
    // print the elements of the vector
    for(int i = 0; i < commands.size(); i++){
        cout << commands[i] << endl;
    }
}

// function that validates the instructions of the entire program and calls the corresponding function for each instruction
void Validate_instructions(vector<string>& commands, int PC)
{
    int i = 0;
    while(!flag && i < commands.size()){
        //initializes the variables that will hold the instruction and its operands
        string instruction = "";
        string rd = "";
        string rs1 = "";
        string rs2 = "";

        //gets number of commas in the line to determine the number of operands in the instruction
        int numCommas = 0;
        for (char c : commands[i]) {
            if (c == ',') {
                numCommas++;
            }
        }

        pair<string, int> offsetAndReg; //pair that holds the offset and the register to be used in the load and store instructions

        //parses the instruction into its components according to the number of operands determined by the number of commas
        if (numCommas == 2){
            parseInputString_3operands(commands[i], instruction, rd, rs1, rs2);
        }
        else if (numCommas == 1){
            parseInputString_2operands(commands[i], instruction, rd, rs1);
        }
        size_t colon_pos = commands[i].find(":");
        //calls the corresponding function according to the instruction
        if (instruction == "LUI" || instruction == "lui") {
            lui(rd, stoi(rs1));
        }
        else if (instruction == "AUIPC" || instruction == "auipc") {
            auipc(rd, stoi(rs1));
        }
        else if (instruction == "JAL" || instruction == "jal") {
            cout << "RS1: " << rs1 << endl;
            int imm = labels[rs1];
            jal(rd, imm, i);
        }
        else if (instruction == "JALR" || instruction == "jalr") {
            offsetAndReg = getOffset(rs1);
            jalr(rd, offsetAndReg.second, offsetAndReg.first, i);
        }
        else if (instruction == "BEQ" || instruction == "beq") {
            int imm = labels[rs2];
            beq(rd, rs1, imm, i);
        }
        else if (instruction == "BNE" || instruction == "bne") {
            int imm = labels[rs2];
            bne(rd, rs1, imm, i);
        }
        else if (instruction == "BLT" || instruction == "blt") {
            int imm = labels[rs2];
            blt(rd, rs1, imm, i);
        }
        else if (instruction == "BGE" || instruction == "bge") {
            int imm = labels[rs2];
            bge(rd, rs1, imm, i);
        }
        else if(instruction == "BLE" || instruction == "ble"){
            int imm = labels[rs2];
            ble(rd, rs1, imm, i);
        }
        else if (instruction == "BLTU" || instruction == "bltu") {
            int imm = labels[rs2];
            bltu(rd, rs1, imm, i);
        }
        else if (instruction == "BGEU" || instruction == "bgeu") {
            int imm = labels[rs2];
            bgeu(rd, rs1, imm, i);
        }
         else if (instruction == "LW" || instruction == "lw") {
            offsetAndReg = getOffset(rs1);
            LW(rd, offsetAndReg.second, offsetAndReg.first);
        }
         else if (instruction == "LH" || instruction == "lh") {
            offsetAndReg = getOffset(rs1);
            LH(rd, offsetAndReg.second, offsetAndReg.first);
        }
        else if (instruction == "LB" || instruction == "lb") {
            offsetAndReg = getOffset(rs1);
            LB(rd, offsetAndReg.second, offsetAndReg.first);
        }
       
        else if (instruction == "LBU"   || instruction == "lbu") {
            offsetAndReg = getOffset(rs1);
            LBU(rd, offsetAndReg.second, offsetAndReg.first);
        }
        else if (instruction == "LHU" || instruction == "lhu") {
            offsetAndReg = getOffset(rs1);
            LHU(rd, offsetAndReg.second, offsetAndReg.first);
        }
        else if (instruction == "SB" || instruction == "sb") {
            offsetAndReg = getOffset(rs1);
            SB(rd, offsetAndReg.second, offsetAndReg.first);
        }
        else if (instruction == "SH" || instruction == "sh") {
            offsetAndReg = getOffset(rs1);
            SH(rd, offsetAndReg.second, offsetAndReg.first);
        }
        
        else if (instruction == "SW" || instruction == "sw") {
            offsetAndReg = getOffset(rs1);
            SW(rd, offsetAndReg.second, offsetAndReg.first);
        }
        else if (instruction == "ADDI" || instruction == "addi") {
            cout << rd << " " << rs1 << " " << rs2 << endl;
            ADDI(rd, rs1, stoi(rs2));
        }
        else if (instruction == "SLTI" || instruction == "slti") {
            SLTI(rd, rs1, stoi(rs2));
        }
        else if (instruction == "SLTIU" || instruction == "sltiu") {
            SLTIU(rd, rs1, stoi(rs2));
        }
        else if (instruction == "XORI" || instruction == "xori") {
            XORI(rd, rs1, stoi(rs2));
        }
        else if (instruction == "ORI" || instruction == "ori") {
            ORI(rd, rs1, stoi(rs2));
        }
        else if (instruction == "ANDI" || instruction == "andi") {
            ANDI(rd, rs1, stoi(rs2));
        }
        else if (instruction == "SLLI" || instruction == "slli") {
            slli(rd, rs1, stoi(rs2));
        }
        else if (instruction == "SRLI" || instruction == "srli") {
            srli(rd, rs1, stoi(rs2));
        }
        else if (instruction == "SRAI" || instruction == "srai") {
            srai(rd, rs1, stoi(rs2));
        }
        else if (instruction == "ADD" || instruction == "add") {
            add(rd, rs1, rs2);
        }
        else if (instruction == "SUB" || instruction == "sub") {
            sub(rd, rs1, rs2);
        }
        else if (instruction == "SLL" || instruction == "sll") {
            sll(rd, rs1, rs2);
        }
        else if (instruction == "SLT" || instruction == "slt") {
            slt(rd, rs1, rs2);
        }
        else if (instruction == "SLTU" || instruction == "sltu") {
            sltu(rd, rs1, rs2);
        }
        else if (instruction == "XOR" || instruction == "xor") {
            XOR(rd, rs1, rs2);
        }
        else if (instruction == "SRL" || instruction == "srl") {
            srl(rd, rs1, rs2);
        }
        else if (instruction == "SRA" || instruction == "sra") {
            sra(rd, rs1, rs2);
        }
        else if (instruction == "OR" || instruction == "or") {
            OR(rd, rs1, rs2);
        }
        else if (instruction == "AND" || instruction == "and") {
            AND(rd, rs1, rs2);
        }
        else if (commands[i] == "fence" || commands[i] == "ecall" || commands[i] == "ebreak") {
            cout << "INSTRUCTION NUM: "  << i << endl;
            if (flag == false) {
                cout << "Memory Map" << endl;
                for (auto it = mem.begin(); it != mem.end(); it++) {
                    cout << it->first << " " << it->second << endl;
                }
                cout << endl;
            }
            else {
                cout << "Error in line " << i << endl;
                cout << commands[i] << endl;
                break;
            }
            // print register map
            cout << "Register Map" << endl;
            for (auto it = registers.begin(); it != registers.end(); it++) {
                cout << it->first << ": Decimal value " << it->second <<  ", Hexadecimal value: " << dec2Hexadecimal(it->second) << ", Binary value: " << decToBin(it->second) << endl;
            }

            cout << "Current Instruction: " << commands[i] << endl;
            cout << "PC: " << PC + i << endl;
            cout << "Program terminated sucessfully\n";
            exit(0);
        }
        else if(colon_pos == string::npos){
            cout << "Invalid Instruction " << instruction << endl;
            flag = true;
        }

        // print memory map
        cout << "INSTRUCTION NUM: "  << i << endl;

        if (flag == false) {
            cout << "Memory Map" << endl;
            for (auto it = mem.begin(); it != mem.end(); it++) {
                cout << it->first << " " << it->second << endl;
            }
            cout << endl;
        }
        else {
            cout << "Error in line " << i << endl;
            cout << commands[i] << endl;
            break;
        }

        // print register map
        cout << "Register Map" << endl;
        for (auto it = registers.begin(); it != registers.end(); it++) {
            cout << it->first << ": Decimal value " << it->second <<  ", Hexadecimal value: " << dec2Hexadecimal(it->second) << ", Binary value: " << decToBin(it->second) << endl;
        }

        cout << "Current Instruction: " << commands[i]<<endl;
        cout << "PC: " << PC + i << endl;

        i++;


        if(registers["zero"]!=0){
            break;
        }
  
    }
   
}

// MAIN FUNCTION
int main(){
    // INITIALIZING THE REGISTERS
    registers["x0"] = 0; 
    registers["x1"] = 0;
    registers["sp"] = 2047;
    registers["gp"] = 0;
    registers["tp"] = 0;
    registers["t0"] = 0;
    registers["t1"] = 0;
    registers["t2"] = 0;
    registers["t3"] = 0;
    registers["t4"] = 0;
    registers["t5"] = 0;
    registers["t6"] = 0;
    registers["a0"] = 0;
    registers["a1"] = 0;
    registers["a2"] = 0;
    registers["a3"] = 0;
    registers["a4"] = 0;
    registers["a5"] = 0;
    registers["a6"] = 0;
    registers["a7"] = 0;
    registers["s0"] = 0;
    registers["s1"] = 0;
    registers["s2"] = 0;
    registers["s3"] = 0;
    registers["s4"] = 0;
    registers["s5"] = 0;
    registers["s6"] = 0;
    registers["s7"] = 0;
    registers["s8"] = 0;
    registers["s9"] = 0;
    registers["s10"] = 0;
    registers["s11"] = 0;

    //inputting the file name and validating its content.
    string filename;
    cout << "Enter the name of the file: ";
    cin >> filename;
    vector<string> instructions;

    string dataFileName; 
    cout << "Enter the name of the data file: ";
    cin >> dataFileName;

    int PC; 
    cout << "Enter the PC: ";
    cin >> PC;

    cout << endl; 

    readData(mem, dataFileName);

    putIntoVector(filename, instructions);	
    Validate_instructions(instructions, PC);
    registers["x4"] = 2;
    auipc("s8",7);
    cout << mem[registers["t0"]] << endl;
}