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

	//获取文件列表—在主机上设置一个共享目录，这个目录下的文件都要共享给别人 
	static void ShareList(const httplib::Request &req, httplib::Response &rsp){
		//查看目录是否存在， 若不在则创建
		if (!boost::filesystem::exists(SHARED_PATH)){
			boost::filesystem::create_directory(SHARED_PATH);
		}

		boost::filesystem::directory_iterator begin(SHARED_PATH);//实例化目录迭代器开始
		boost::filesystem::directory_iterator end;//实例化目录迭代器结尾
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
		if (req.method == "GET"){
			boost::filesystem::path req_path(req.path);//req.path-客户端请求得资源路径/download/filename.txt
			std::string name = req_path.filename().string();//只获取文件名称 filename.txt
			std::string realpath = SHARED_PATH + name;
			if (!boost::filesystem::exists(realpath) || boost::filesystem::is_directory(realpath)){
				rsp.status = 404;
				return;
			}

			if (req.method == "GET"){
				//以前的Get请求是直接下载完整文件，现在可以分块传输
				//判断是否需要分块传输，就是这次请求中是否有Range同步判断
				if (req.has_header("Range")){
					std::string range_str = req.get_header_value("Range");
					httplib::Ranges ranges;
					httplib::detail::parse_range_header(range_str, ranges);//解析客户端的Range数据
					int64_t range_start = ranges[0].first;
					int64_t range_end = ranges[0].second;
					int range_len = range_end - range_start + 1;
					std::cout << "range" << range_start << "-" << range_end << std::endl;
					FileUtil::ReadRange(realpath, &rsp.body, range_len, range_start);
					rsp.status = 206;
				}
				else{
					//没有Range头部就是完整文件下载
					if (FileUtil::Read(realpath, &rsp.body) == false){
						rsp.status = 500;
						return;
					}
					rsp.status = 200;
					return;
				}
			}
		}
	}

private:
	httplib::Server _srv;
};