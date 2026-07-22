#include<iostream>
using namespace std;
#include<string>;
#include <ctime>//调用时间的头文件
int main()
{
	int i = 0;
	int arr[8] = {120,10,7,4,100,11,22,99 };
	int start = 0;
	
	cout << "排序前" << endl;
	for (i=0;i < 8; i++) {
		cout << arr[i]<<",";
		
	}
	
	

	for (int a = 0;a < 8 - 1;a++) {
		for (int start = 0;start < 8 - a - 1;start++) {

			if (arr[start] > arr[start + 1]) {
				int biaoji = arr[start];
				arr[start] = arr[start + 1];
				arr[start + 1] = biaoji;

			}
		}

	}
		
	cout << endl;
	cout << "排序后" << endl;
	for (i=0;i <8; i++){
		cout << arr[i]<<",";
	}
	
	
		system("pause");

		return 0;

	}
	


