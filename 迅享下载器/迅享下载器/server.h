#pragma once
#include"util.h"
#include"httplib.h"
#include<boost/filesystem.hpp>

#define P2P_PORT 9000
#define SHARED_PATH "./Shared/"

class Server{
public:
	bool Start(){
		_srv.Get("/hostpair", HostPair);
		_srv.Get("/list", ShareList);
		_srv.Get("/download.*", DownLoad);
		_srv.listen("0.0.0.0", P2P_PORT);
		return true;
	}
private:
	static void HostPair(const httplib::Request &req, httplib::Response &rsp){
		rsp.status = 200;
		return;
	} 

	//��ȡ�ļ��б�������������һ������Ŀ¼�����Ŀ¼�µ��ļ���Ҫ��������� 
	static void ShareList(const httplib::Request &req, httplib::Response &rsp){
		//�鿴Ŀ¼�Ƿ���ڣ� �������򴴽�
		if (!boost::filesystem::exists(SHARED_PATH)){
			boost::filesystem::create_directory(SHARED_PATH);
		}

		boost::filesystem::directory_iterator begin(SHARED_PATH);//ʵ����Ŀ¼��������ʼ
		boost::filesystem::directory_iterator end;//ʵ����Ŀ¼��������β
		for (; begin != end; ++begin){
			if (boost::filesystem::is_directory(begin->status())){
				continue;
			}
			std::string name = begin->path().filename().string();
			rsp.body += name + "\r\n";
		}
		rsp.status = 200;
		return;
	}
	static void DownLoad(const httplib::Request &req, httplib::Response &rsp){

		boost::filesystem::path req_path(req.path);//req.path-�ͻ����������Դ·��/download/filename.txt
		std::string name = req_path.filename().string();//ֻ��ȡ�ļ����� filename.txt
		std::string realpath = SHARED_PATH + name;
		if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath)){
			rsp.status = 404;
			return;
		}

		if (req.method == "GET"){
			//��ǰ��Get������ֱ�����������ļ������ڿ��Էֿ鴫��
			//�ж��Ƿ���Ҫ�ֿ鴫�䣬��������������Ƿ���Rangeͬ���ж�
			if (req.has_header("Range")){
				std::string range_str = req.get_header_value("Range");
				int64_t range_start;
				int64_t range_end;
				FileUtil::GetRange(range_str, &range_start, &range_end);

				int64_t range_len = range_end - range_start + 1;

				std::cout << "������յ�Range:" << range_start << "-" << range_end << std::endl;
				FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
				rsp.status = 206;
				std::cout << rsp.body.c_str() << std::endl;
				std::cout << "�������Ӧ�����������\n\n";
			}
			else{
				//û��Rangeͷ�����������ļ�����
				if (FileUtil::Read(realpath, &rsp.body) == false){
					rsp.status = 500;
					return;
				}
				rsp.status = 200;
				return;
			}
		}
		else
		{
			//��������HEAD����Ŀͻ���ֻҪͷ����Ҫ����
			uint64_t filesize = FileUtil::GetFileSize(realpath);
			rsp.set_header("Content-Length", std::to_string(filesize));//������Ӧheaderͷ����Ϣ�ӿ�
			rsp.status = 200;
		}
	}
	

private:
	httplib::Server _srv;
};