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
	boost::filesystem::directory_iterator begin(ptr);//����һ��Ŀ¼������
	boost::filesystem::directory_iterator end;
	for (; begin != end; begin++){
		//begin->status()Ŀ¼�е�ǰ�ļ���״̬��Ϣ
		//boost::filesystem::is_directory()�ж��Ƿ���һ��Ŀ¼
		if (boost::filesystem::is_directory(begin->status())){
			//begin->path().string()��ȡ��ǰ�����ļ����ļ���
			std::cout << begin->path().string() << "��һ��Ŀ¼\n";
		}
		else{
			std::cout << begin->path().string() << "��һ����ͨ�ļ�\n";
			//std::cout << "�ļ���:" << begin->path().filename().string << std::endl;;

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
	//�����߳�Ҫ���пͻ���ģ���Լ������ģ��
	//����һ���߳����пͻ���ģ�飬���߳����з����ģ��
	std::thread thr_client(ClientRun);
	Server srv;
	srv.Start();
	return 0;
} 