#include <iostream>
using namespace std;

class class1{
		
	public:
		int i;
		class1(int a)
		{
			i=a;
		}
		class1()
		{

		}
		int geti()
		{
			cout<<"value in class1  get"<<i;
			return i;
		}
};

class class2{
private:
	class1 obj;
public:
	class2(class1 o)
	{
		obj = o;
	}
	int get(){
		cout<< "value in class2 get"<<obj.geti();
		return obj.geti();
	}
	int change
};


int main()
{
	int i =1;
	class1 o1(1);
	o1.geti();
	class2 o2(o1);
	o2.get();
	return 0;
}