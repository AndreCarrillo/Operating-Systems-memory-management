// assignment3.1.cpp : Defines the entry point for the console application.
//
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <bitset>
#include <istream>
#include <math.h>
#include <queue>
#include <iterator>
#include <algorithm>

using namespace std;
ifstream inputFile;
string inputLine;
vector<string> file;
string segmentNumber, pageNumber, offSet;
string inputLine1;
int pageFrames; /*ALSO IS USED AS DELTA FOR WS*/
int segmentLength; /*max segment length (number of pages)*/
int pageSize;	/*page size in bytes*/
int maxPageFrames; /*number of page frames per process*/
int lookAheadForOpt; /*only for OPT, others is 0*/
int minWS; //minimum number of pages we can have free in cache
int maxWS; //maximum number of pages we can have free in cache
int numberOfProcesses;
const unsigned maxBits = 8;
string s, hexa;
vector<string> processVector;
vector<string> binaryVector;
struct lruStructs {
	int time;
	string binary;
};
lruStructs *lruStruct;
struct lfuStructs {
	int count;
	string binary;
};
lfuStructs *lfuStruct;
struct optStructs {
	int distanceAhead;
	string binary;
};
optStructs *optStruct;
struct segmentTables {
	int table[20][20][20] = { { { 0} } };
	queue<string> queue1;
	vector<lruStructs> lruArray;
	vector<lfuStructs> lfuArray;
	vector<optStructs> optArray;
};

segmentTables *segmentTable;

struct process {
	string processName;
	int numberOfFramesOnDisk;
	int segID;
};
process *processes;

void read_input() {

	inputFile >> pageFrames >> segmentLength >> pageSize >> maxPageFrames >> lookAheadForOpt >> minWS >> maxWS >> numberOfProcesses;

	processes = new process[numberOfProcesses];

	getline(inputFile, inputLine);

	for (int i = 0; i < numberOfProcesses; i++) {
		getline(inputFile, inputLine);
		stringstream iss(inputLine);
		iss >> processes[i].processName >> processes[i].numberOfFramesOnDisk;
		processes[i].segID = i;
	}
}
void convert_to_binary() {
	while (getline(inputFile, inputLine)) {
		file.push_back(inputLine);
	}
	for (int i = 0; i < file.size(); i++) {
		stringstream line(file[i]);
		getline(line, hexa, ' ');
		processVector.push_back(hexa);

		getline(line, hexa, ' ');
		
		if (hexa.find('-') != string::npos) {
			binaryVector.push_back("-1");
		}
		if (hexa.find('x') != string::npos) {
			size_t xPlace = hexa.find('x');
			string temp = hexa.substr(xPlace + 1);
			istringstream iss(temp);
			int i;
			iss >> hex >> i;
			bitset<8> bs = i;

			binaryVector.push_back(bs.to_string<char, char_traits<char>, allocator<char> >());
		}

	}

}

int extract_segment_number(int i, string binaryAddress) {
	segmentNumber = binaryAddress.substr(0, log2((processes[i].numberOfFramesOnDisk / segmentLength)));
	int dec = 0;
	reverse(segmentNumber.begin(), segmentNumber.end());
	for (int i = 0; i < segmentNumber.size(); ++i) {
		dec += (int(segmentNumber[i]) - 48)*pow(2, i);
	}
	return dec;
}

int extract_page_number(int i, string binaryAddress) {
	pageNumber = binaryAddress.substr(log2((processes[i].numberOfFramesOnDisk / segmentLength)), log2(segmentLength));
	
	int dec = 0;
	reverse(pageNumber.begin(), pageNumber.end());
	for (int i = 0; i < pageNumber.size(); ++i) {
		dec += (int(pageNumber[i]) - 48)*pow(2, i);
	}
	return dec;
}
int extract_offset(int i, string binaryAddress) {
	offSet = binaryAddress.substr(log2(pageSize));
	
	int dec = 0;
	reverse(offSet.begin(), offSet.end());
	for (int i = 0; i < offSet.size(); ++i) {
		dec += (int(offSet[i]) - 48)*pow(2, i);
	}
	return dec;
}
void fifo() {
	segmentTable = new segmentTables[numberOfProcesses];
	
	vector<int> pagefaultforfifo(numberOfProcesses);
	for (int i = 0; i < processVector.size(); i++) {
			
		if (binaryVector[i] != "-1") {
			string pid = processVector[i].substr(2, 1);
			int segNumber = extract_segment_number(stoi(pid), binaryVector[i]);
			int pageNumber = extract_page_number(stoi(pid), binaryVector[i]);
			int offSet = extract_offset(stoi(pid), binaryVector[i]);
			
			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 0) {
				pagefaultforfifo[stoi(pid)]++;

				if (segmentTable[stoi(pid)].queue1.size() == maxPageFrames) {
					
					segmentTable[stoi(pid)].queue1.front();
					string temp1, temp2, temp3;
					stringstream ss(segmentTable[stoi(pid)].queue1.front());
					getline(ss,temp1, ',');
					
					getline(ss, temp2, ',');
					
					getline(ss, temp3);
					
					segmentTable[stoi(pid)].table[stoi(temp1)][stoi(temp2)][stoi(temp3)] = 0;
					segmentTable[stoi(pid)].queue1.pop();
				}
				
				segmentTable[stoi(pid)].queue1.push(to_string(segNumber) + "," + to_string(pageNumber) + "," + to_string(offSet));
				segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] = 1;

			}
		}
		}
	cout << ".+-^-+. FIFO Page Replacement .+-^-+. " << endl << endl;
	for (int k = 0; k < numberOfProcesses; k++) {
		cout << "Process: " + to_string(k) + ':' << endl;
		cout << pagefaultforfifo[k] << endl;
	}
	cout << endl;
	
}

void lru() {
	vector<int> pagefaultforlru(numberOfProcesses);
	segmentTable = new segmentTables[numberOfProcesses];
	lruStruct = new lruStructs[binaryVector.size()];

	for (int i = 0; i < binaryVector.size(); i++) {
		lruStruct[i] = { i,binaryVector[i] };
	}
	for (int j = 0; j < binaryVector.size(); j++) {
		if (binaryVector[j] != "-1") {
			string pid = processVector[j].substr(2, 1);
			int segNumber = extract_segment_number(stoi(pid), binaryVector[j]);
			int pageNumber = extract_page_number(stoi(pid), binaryVector[j]);
			int offSet = extract_offset(stoi(pid), binaryVector[j]);

			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 1) {
				for (int l = 0; l < maxPageFrames; l++) {

					if (segmentTable[stoi(pid)].lruArray[l].binary == lruStruct[j].binary) {
						segmentTable[stoi(pid)].lruArray[l] = lruStruct[j];
						}
				}
			}

			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 0) {
				pagefaultforlru[stoi(pid)]++;

				if (segmentTable[stoi(pid)].lruArray.size() == maxPageFrames) {
					int min = segmentTable[stoi(pid)].lruArray[0].time;
					int count = 0;
					for (int i = 0; i < segmentTable[stoi(pid)].lruArray.size(); i++) {
						if (segmentTable[stoi(pid)].lruArray[i].time < min) {
							min = segmentTable[stoi(pid)].lruArray[i].time;
							count = i;
						}
			
					}
					int tempA, tempB, tempC;
					tempA = extract_segment_number(stoi(pid), segmentTable[stoi(pid)].lruArray[count].binary);
					tempB = extract_page_number(stoi(pid), segmentTable[stoi(pid)].lruArray[count].binary);
					tempC = extract_offset(stoi(pid), segmentTable[stoi(pid)].lruArray[count].binary);

					segmentTable[stoi(pid)].table[tempA][tempB][tempC] = 0;
					segmentTable[stoi(pid)].lruArray.erase(segmentTable[stoi(pid)].lruArray.begin() + count);

				}
				segmentTable[stoi(pid)].lruArray.push_back(lruStruct[j]);
				segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] = 1;
			}
		}

	}
	cout << ".+-^-+. LRU Page Replacement .+-^-+. " << endl << endl;
	for (int k = 0; k < numberOfProcesses; k++) {
		cout << "Process: " + to_string(k) + ':' << endl;
		cout << pagefaultforlru[k] << endl;
	}
	cout << endl;
	}

void lfu() {
	vector<int> pagefaultforlfu(numberOfProcesses);
	segmentTable = new segmentTables[numberOfProcesses];
	lfuStruct = new lfuStructs[binaryVector.size()];

	//initialize lfuStruct
	for (int i = 0; i < binaryVector.size(); i++) {
		lfuStruct[i] = { 0,binaryVector[i] };
	}

	//go through binaryvector and perform lfu
	for (int j = 0; j < binaryVector.size(); j++) {
		
		if (binaryVector[j] != "-1") {
			string pid = processVector[j].substr(2, 1);
			int segNumber = extract_segment_number(stoi(pid), binaryVector[j]);
			int pageNumber = extract_page_number(stoi(pid), binaryVector[j]);
			int offSet = extract_offset(stoi(pid), binaryVector[j]);
			//if segTable is already 1, (in cache), increase counter of lfu struct.count
			//find place in cache and increment it's counter by 1
			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 1) {
				for (int l = 0; l < maxPageFrames; l++) {

					if (segmentTable[stoi(pid)].lfuArray[l].binary == lfuStruct[j].binary) {

						segmentTable[stoi(pid)].lfuArray[l].count++;

					}
				}
			}
			//if not in cache, push back into cache
			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 0) {
				pagefaultforlfu[stoi(pid)]++;
				//if cache is full, find element with lowest count, and and delete it
				if (segmentTable[stoi(pid)].lfuArray.size() == maxPageFrames) {

					int min = segmentTable[stoi(pid)].lfuArray[0].count;
					int counter = 0;

					for (int i = 0; i < segmentTable[stoi(pid)].lfuArray.size(); i++) {
						if (segmentTable[stoi(pid)].lfuArray[i].count < min) {
							min = segmentTable[stoi(pid)].lfuArray[i].count;
							counter = i;
						}

					}
					int tempA, tempB, tempC;
					tempA = extract_segment_number(stoi(pid), segmentTable[stoi(pid)].lfuArray[counter].binary);
					tempB = extract_page_number(stoi(pid), segmentTable[stoi(pid)].lfuArray[counter].binary);
					tempC = extract_offset(stoi(pid), segmentTable[stoi(pid)].lfuArray[counter].binary);

					segmentTable[stoi(pid)].table[tempA][tempB][tempC] = 0;
				
					segmentTable[stoi(pid)].lfuArray.erase(segmentTable[stoi(pid)].lfuArray.begin() + counter);
				

				}
				//pushback struct into vector and set validity bit to 1
				segmentTable[stoi(pid)].lfuArray.push_back(lfuStruct[j]);
				segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] = 1;

			}
		}
	}
	cout << ".+-^-+. LFU Page Replacement .+-^-+. " << endl << endl;
	for (int k = 0; k < numberOfProcesses; k++) {
		cout << "Process: " + to_string(k) + ':' << endl;
		cout << pagefaultforlfu[k] << endl;
	}
	cout << endl;

}
void opt() {
	vector<int> pagefaultforopt(numberOfProcesses);
	segmentTable = new segmentTables[numberOfProcesses];
	optStruct = new optStructs[binaryVector.size()];

	for (int i = 0; i < binaryVector.size(); i++) {
		optStruct[i].distanceAhead = 0;
		optStruct[i].binary = binaryVector[i];
	}

	for (int i = 0; i < binaryVector.size(); i++) {
		if (binaryVector[i] != "-1") {
			string pid = processVector[i].substr(2, 1);
			int segNumber = extract_segment_number(stoi(pid), binaryVector[i]);
			int pageNumber = extract_page_number(stoi(pid), binaryVector[i]);
			int offSet = extract_offset(stoi(pid), binaryVector[i]);
			//if not in cache
			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 0) {
				//increase pagefaultcount per process
				pagefaultforopt[stoi(pid)]++;

				if (segmentTable[stoi(pid)].optArray.size() == maxPageFrames) {
					while (i + lookAheadForOpt > binaryVector.size()) {
						lookAheadForOpt--;
					}
					for (int k = 0; k < maxPageFrames; k++) {
						for (int j = i; j < i + lookAheadForOpt; j++) {
							if (segmentTable[stoi(pid)].optArray[k].binary == binaryVector[j]) {
								segmentTable[stoi(pid)].optArray[k].distanceAhead++;
							}
						}
					}
					int max = segmentTable[stoi(pid)].optArray[0].distanceAhead;
					int index = 0;
					for (int h = 0; h < maxPageFrames; h++) {
						if (segmentTable[stoi(pid)].optArray[h].distanceAhead > max) {
							max = segmentTable[stoi(pid)].optArray[h].distanceAhead;
							index = h;
						}
					}

					int tempA, tempB, tempC;
					tempA = extract_segment_number(stoi(pid), segmentTable[stoi(pid)].optArray[index].binary);
					tempB = extract_page_number(stoi(pid), segmentTable[stoi(pid)].optArray[index].binary);
					tempC = extract_offset(stoi(pid), segmentTable[stoi(pid)].optArray[index].binary);

					segmentTable[stoi(pid)].table[tempA][tempB][tempC] = 0;
					//before i erase i need to set the distance ahead to 0 again
					segmentTable[stoi(pid)].optArray[index].distanceAhead = 0;
					segmentTable[stoi(pid)].optArray.erase(segmentTable[stoi(pid)].optArray.begin() + index);
				}
				//pushback into array
				//update validity bit to 1
				segmentTable[stoi(pid)].optArray.push_back(optStruct[i]);
				segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] = 1;

			}

		}

	}
	cout << ".+-^-+. Optimal Page Replacement .+-^-+. " << endl << endl;
	for (int k = 0; k < numberOfProcesses; k++) {

		cout << "Process: " + to_string(k) + ':' << endl;
		cout << pagefaultforopt[k] << endl;
	}
	cout << endl;
}
void workingSet() {
	segmentTable = new segmentTables[numberOfProcesses];
	vector<string> cacheVector;
	int totalPageFaults = 0;

	for (int i = 0; i < binaryVector.size(); i++) {
		if (binaryVector[i] != "-1") {
			string pid = processVector[i].substr(2, 1);
			int segNumber = extract_segment_number(stoi(pid), binaryVector[i]);
			int pageNumber = extract_page_number(stoi(pid), binaryVector[i]);
			int offSet = extract_offset(stoi(pid), binaryVector[i]);
			//if not in cache
			if (segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] == 0) {
				totalPageFaults++;
				if (cacheVector.size() < maxPageFrames) {
					cacheVector.push_back(binaryVector[i]);
				}
				else if (cacheVector.size() == maxPageFrames) {
					int tempA, tempB, tempC;
					tempA = extract_segment_number(stoi(pid), cacheVector[0]);
					tempB = extract_page_number(stoi(pid), cacheVector[0]);
					tempC = extract_offset(stoi(pid), cacheVector[0]);

					segmentTable[stoi(pid)].table[tempA][tempB][tempC] = 0;
					cacheVector.erase(cacheVector.begin());
				}
				
				segmentTable[stoi(pid)].table[segNumber][pageNumber][offSet] = 1;
			}
			
			
		}
	}

	cout << "Total Working Set Page Faults: " + to_string(totalPageFaults) << endl << endl;
	cout << "Working Set: (";
	for (int i = 0; i < cacheVector.size(); i++) {
		if (i + 1 == cacheVector.size()) {
			cout << cacheVector[i];
		}
		else
			cout << cacheVector[i] + ", ";
	}
	cout << ")" << endl;
}


int main(int argc, char* argv[3]) {
	inputLine1 = argv[1];
	
	inputFile.open(inputLine1.c_str());
	read_input();
	convert_to_binary();
	fifo();
	lru();
	lfu();
	opt();
	workingSet();
	getchar();
    return 0;

}

