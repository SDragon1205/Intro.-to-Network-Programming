#include <iostream>
#include <cstdio>
#include <string.h>
#include <algorithm>
#include <fstream> 

using namespace std;

//-------------------------Input file example.txt-------------------------

//-------------------------End of input file example.txt-------------------------


int main(int argc, char *argv[])
{
	string s1, s2;
	char s3[99999];
	

	if(argc != 3)
	{
		cout << "usage: ./main [input file path] [split token]\n";
		return 0;
	}


	ifstream fin;
    fin.open(argv[1]);     
	cout << "-------------------------Input file " << argv[1] << "-------------------------\n";
	s2 = "reverse";
	while(fin.peek() != EOF)
	{
		//cout << "peek: " << fin.peek() << "\n";
		fin >> s1 >> s3;
		cout << s1 << " " << s3 << "\n";
		if(s1 == s2)
		{
			s1 = s3;
			reverse(s1.begin(), s1.end());
			cout << s1 << "\n";
		}
		else
		{
			char *pch = strtok(s3, argv[2]);
			while(pch != NULL)
			{
				printf("%s ", pch);
				pch = strtok(NULL, argv[2]);
			}
			cout << "\n";
		}
		fin.ignore();
	}
	cout << "-------------------------End of input file " << argv[1] << "-------------------------\n";
	cout << "*****************************User input*****************************\n";
	fin.close(); 
	
	cin >> s1;
	while(s1 != "exit")
	{
		cin >> s3;
		if(s1 == s2)
		{
			s1 = s3;
			reverse(s1.begin(), s1.end());
			cout << s1 << "\n";
		}
		else
		{
			char *pch = strtok(s3, argv[2]);
			while(pch != NULL)
			{
				printf("%s ", pch);
				pch = strtok(NULL, argv[2]);
			}
			cout << "\n";
		}
		cin >> s1;
	}
}
