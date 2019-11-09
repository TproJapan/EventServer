#include <thread>
#include <string>

using namespace std;

//ただの関数渡し
void foo() {
  printf("mainとは別スレッドで実行されています\n");
}

//引数あり、構造体での関数オブジェクト
struct bar1 {
  void operator() (const string& msg) const {
    printf("%sが渡されました\n",msg.c_str());
  }
};

//引数なし、クラスでの関数オブジェクト
//publicはつけなくてもデフォルトでpublic。privateだとコンパイルエラー
class bar2 {
  public: void operator() () {
    printf("引数なしです\n");
  }
};

//引数あり、クラスでの関数オブジェクト
class bar3 {
  string x;
	public:
		bar3(string y){
      x = y;
    };
		~bar3(){};
  public: void operator()(){
    printf("%sが渡されました\n",x.c_str());
  }
};

int main() {
  thread th1(foo);//ただの関数渡し
  thread th2(bar1(), "An argument");//構造体メゾットを直接実行
  thread th3{bar2()};//クラスメゾットを直接実行
  thread th4{bar3("An argument")};//クラスメゾットを直接実行

  th1.join();
  th2.join();
  th3.join();
  th4.join();
  return 0;
}