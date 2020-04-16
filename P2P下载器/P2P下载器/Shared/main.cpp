#include"util.h"
#include"httplib.h"
#include"client.h"

void helloworld(const httplib::Request &req,httplib::Response &rsp){
	printf("httplib server recv a req:%s\n",req.path.c_str());
	rsp.set_content("<html><hl>HelloWorld</hl></html>", "texl/html");
	rsp.status = 200;
}
void Scander(){
	const char *ptr = "./";
	boost::filesystem::directory_iterator begin(ptr);//定义一个目录迭代器
	boost::filesystem::directory_iterator end;
	for (; begin != end; begin++){
		//begin->status()目录中当前文件的状态信息
		//boost::filesystem::is_directory()判断是否是一个目录
		if (boost::filesystem::is_directory(begin->status())){
			//begin->path().string()获取当前迭代文件的文件名
			std::cout << begin->path().string() << "是一个目录\n";
		}
		else{
			std::cout << begin->path().string() << "是一个普通文件\n";
			//std::cout << "文件名:" << begin->path().filename().string << std::endl;;

		}
	}
}

void test(){
	/*std::vector<Adapter> list;
	AdapterUtil::GetAllAdapter(&list);
	Sleep(1000000);*/

	/*httplib::Server srv;
	srv.Get("/", helloworld);
	srv.listen("0.0.0.0", 9000);
	return 0;*/

	/*Scander();
	Sleep(10000000);*/
}
void ClientRun(){
	Client cli;
	cli.Start();
} 

int main(int argc,char *argv[]){
	//在主线程要运行客户端模块以及服务端模块
	//创建一个线程运行客户端模块，主线程运行服务端模块
	std::thread thr_client(ClientRun);
	Server srv;
	srv.Start();
	return 0;
} 