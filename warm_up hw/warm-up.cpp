#include <iostream>
#include <cstdlib>
#include <fstream>
#include <cstring>
using namespace std;

void reverse(char string[]){
	for(int i=strlen(string)-1;i>=0;i--) 
		cout<<string[i];
	cout<<endl;
}

void split(char string[], char str[]){
	bool check = false;
	bool con = false;
	for(int i=0;i<strlen(string);i++){
		int j;
		for(j=0;j<strlen(str);j++){
			if(string[i+j]==str[j])
				check = true;	
			else {
				check = false;
				break;
			}
		}
		if(check){
			if(!con) cout<<" ";
			con = true;
			i+=(j-1);
			check = false;
			continue;
		}
		else con = false;
		
		cout<<string[i];
	}
	cout<<endl;
}

int main(int argc, char** argv){
	ifstream file(argv[1]);
	
	char sp[10000];
	strcpy(sp, argv[2]);
	char choice[100];
	if(file>>choice);
	else cin>>choice;
	while(strcmp(choice, "exit")){
		char string[10000];
		
		if(!strcmp(choice, "reverse")){
			if(file.getline(string, sizeof(string),'\n'));
			else cin.getline(string, 10000);
			reverse(string);
		}
			
		else if(!strcmp(choice, "split")){
			if(file>>string);
			else cin>>string;
			split(string, sp);
		}
			
		
		if(file>>choice);
		else cin>>choice;
	}
	return 0;
}