#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace webui
{
	static const size_t BUF_SIZE = 4 * 1024 * 1024; // 4 MB

	class widget
	{
	public:
		explicit widget(std::string title = ""): title_(title)
		{
			id_ = generate_id();
			map_.insert(std::make_pair(id_, this));
		}
		virtual ~widget() { }

		const std::string& get_title() const { return title_; }
		void set_title(const std::string& s) { title_ = s; }

		virtual std::string render() { return "<div id=\"" + id_ + "\">" + title_ + "</div>"; }

		static widget* find(std::string name)
		{
			auto it = map_.find(name);
			return (it != map_.end() ? it->second : nullptr);
		}
		static std::unordered_map<std::string, widget*> map_;

		void do_event(std::string action)
		{
			if (action == "click") {
				std::cerr << "[DEBUG] widget '" << id_ << "' clicked!" << std::endl;
			}
			auto it = events_.find(action);
			if (it != events_.end()) {
				it->second();
			}
		}

		void set_event(std::string action, std::function<void(void)> fn)
		{
			events_.insert(std::make_pair(action, fn));
		}

		static void start_capture_widget_titles()
		{
			recorded_titles_.clear();
			for (auto it = map_.begin(); it != map_.end(); ++it) {
				recorded_titles_[it->first] = it->second->title_;
			}
		}
		static std::string get_changed_widget_titls()
		{
			std::string s;
			for (auto it = map_.begin(); it != map_.end(); ++it) {
				if (recorded_titles_[it->first] != it->second->title_) {
					if (!s.empty()) s += ",";
					s += "{\"id\":\"" + it->first + "\",\"title\":\"" + it->second->title_ + "\"}";
				}
			}
			return "[" + s + "]";
		}
	private:
		static std::unordered_map<std::string, std::string> recorded_titles_;

	protected:
		std::string get_id() const { return id_; }
	private:
		static std::string generate_id()
		{
			static int counter = 0;
			return "webui_widget_" + std::to_string(++counter);
		}

		std::string id_;
		std::string title_;
		std::unordered_map<std::string, std::function<void(void)>> events_;

		widget(const widget&) = delete;
		widget& operator = (const widget&) = delete;
	};
	std::unordered_map<std::string, widget*> widget::map_;
	std::unordered_map<std::string, std::string> widget::recorded_titles_;

	class window: public widget
	{
	public:
		explicit window(std::string title = ""): widget(title) { }

		virtual std::string render()
		{
			std::string html = "<div>"
				"<h1>" + get_title() + "</h1>"
				"<hr>";
			for (auto w : children_) {
				html += w->render();
			}
			html += "</div>";
			return html;
		}

		window& operator += (widget& w)
		{
			children_.push_back(&w);
			return *this;
		}
	private:
		std::vector<widget*> children_;
	};

	class textbox: public widget
	{
	public:
		explicit textbox(std::string title = ""): widget(title) { }

		void set_text(std::string s) {
			std::cerr << "textbox title is now '" << get_title() << "'" << std::endl;
			set_title(s);
			std::cerr << "textbox title changed to '" << get_title() << "'" << std::endl;
		}
	};

	class button: public widget
	{
	public:
		explicit button(std::string title = "Button"): widget(title) { }

		virtual std::string render()
		{
			std::string html = "<button id=\"" + get_id() + "\">" + get_title() + "</button>";
			return html;
		}
	};

	std::string build_html(window& w)
	{
		std::string id = "webui_widget_2";
		return "<!DOCTYPE html>"
			"<html>"
			"<head>"
			"<meta charset=\"utf-8\">"
			"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
			"<title>" + w.get_title() + "</title>"
			"<style>"
			"body,h1,h2,h3,h4,h5,h6,p,div,span,table,tr,td,img{margin:0;padding:0;font-size:32px}"
			".main{margin:0 auto;width:800px}"
			".main h1,.main h2,.main p,.main div{text-align:center}"
			"</style>"
			"</head>"
			"<body>"
			"<div class=\"main\">" + w.render() + "</div>"
			"<script>"
			"var xmlhttp;"
			"if(window.XMLHttpRequest){"
			"xmlhttp=new XMLHttpRequest();"
			"}else{"
			"xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\");"
			"}"
			"xmlhttp.open(\"GET\",\"/action/body/load\",true);"
			"xmlhttp.send();"
			"function button_clicked(){"
			"xmlhttp.onreadystatechange=function(){"
			"if(!(xmlhttp.readyState==4&&xmlhttp.status==200))return false;"
			"const o=(function(s){try{return JSON.parse(s);}catch(e){return false;}})(xmlhttp.responseText);"
			"if(!o)return false;"
			"for(i=0;i<o.response.length;++i){"
			"x=o.response[i];"
			"document.getElementById(x.id).innerHTML=x.title;"
			"}};"
			"xmlhttp.open(\"GET\",\"/action/\" + this.id + \"/click\",true);"
			"xmlhttp.send();"
			"}"
			"document.getElementById(\"webui_widget_3\").onclick=button_clicked;"
			"</script>"
			"</body>"
			"</html>";
	}

	int run(window& w, unsigned short port)
	{
		int s = socket(AF_INET, SOCK_STREAM, 0);

		int enable = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

		struct sockaddr_in addr = { };
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);

		bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr_in));
		listen(s, 10);

		std::cerr << "Visit http://localhost:" << port << "/ in web browser" << std::endl;
		std::vector<char> buf(BUF_SIZE);
		for (;;) {
			socklen_t len = sizeof(struct sockaddr_in);
			int cs = accept(s, reinterpret_cast<struct sockaddr*>(&addr), &len);
			if (cs == -1) break;
			std::string client_ip = inet_ntoa(addr.sin_addr);
			std::cerr << "[DEBUG] client '" << client_ip << "' connected" << std::endl;

			char* p = &buf[0];
			int n = recv(cs, p, BUF_SIZE, 0);
			std::cerr << "[DEBUG] received " << n << " byte(s) from client '" << client_ip << "'" << std::endl;

			std::string request;
			if (*p == 'G' && *(p+1) == 'E' && *(p+2) == 'T' && *(p+3) == ' ') {
				for (p += 4; *p && *p != ' '; ++p) { request += *p; }
			}
			std::cerr << "[DEBUG] client '" << client_ip << "' requested '" << request << "'" << std::endl;

			std::string response;
			std::string content_type;
			std::string text;
			if (request.substr(0, 8) == "/action/") {
				std::string name = request.substr(8);
				std::string::size_type pos = name.find('/');
				std::string action = name.substr(pos + 1);
				name = name.substr(0, pos);
				content_type = "text/json";
				widget::start_capture_widget_titles();
				widget* w = widget::find(name);
				if (w) { w->do_event(action); }
				text = "{\"name\":\"" + name + "\",\"action\":\"" + action + "\", \"response\":";
				text += widget::get_changed_widget_titls();
				text += "}";
			} else if (request == "/") {
				content_type = "text/html";
				text = build_html(w);
			}
			if (text.empty()) {
				response = "HTTP/1.1 404 Not Found\r\n";
			} else {
				response = "HTTP/1.1 200 OK\r\n";
				response += "Content-Length: " + std::to_string(text.size()) + "\r\n";
				response += "Content-Type: " + content_type + "\r\n";
				response += "\r\n";
				response += text;
			}

			for (size_t pos = 0; pos < response.size(); ) {
				int n = send(cs, response.c_str() + pos, response.size() - pos, 0);
				if (n <= 0) break;
				pos += n;
			}
			std::cerr << "[DEBUG] send back " << response.size() << " byte(s) to client '" << client_ip << "'" << std::endl;
			close(cs);
		}
		close(s);
		return 0;
	}
}
