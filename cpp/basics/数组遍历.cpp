#include<iostream>
using namespace std;
#include<string>;
#include <ctime>//调用时间的头文件
int main()
{
	
	int arr[2][3]{
		 {1,2,3 }, 
         {4,5,6}
	};
	for (int a = 0;a < 2;a++) {
		for (int b = 0;b < 3;b++) {
			cout << arr[a][b]<<" ";
			
		}
		cout << endl;
	}
	system("pause");

	return 0;

}


