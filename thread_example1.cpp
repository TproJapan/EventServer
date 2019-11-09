#include <iostream>
#include <thread>
#include <mutex>
#include <string>

using namespace std;

mutex printMutex_;
void print(const string& s) {
  lock_guard<mutex> lk(printMutex_);
  cout << s << endl;
}

//引数ありの関数渡し
void foo(int a) {
  	print("[foo]mainとは別スレッドで実行されています");
	printf("a=%d",a);
}

//引数なし、構造体での関数オブジェクト
struct bar2 {
  void operator()() {
    printf("[foo]mainとは別スレッドで実行されています\n");
  }
};

//引数なし、クラスでの関数オブジェクト
//コンストラクタ、デストラクタは定義しない場合はデフォルトのものが適用される
//publicはつけなくてもデフォルトでpublic。privateだとコンパイルエラー
class Bar{
	public:
		Bar(){};
		~Bar(){};
	public:void operator()(){
  		printf("[bar]mainとは別スレッドで実行されています");
	}
};

int main()
{
	thread th1(foo,1);//引数あり関数渡し
	
	//関数オブジェクトインスタンス渡して実行
	bar2 bar2;
	thread th2(bar2);
	
	//関数オブジェクトインスタンスを渡して実行
	Bar	bar3;
	thread th3(bar3);
	
	th1.join();
	th2.join();
	th3.join();

	return 0;
}

