#include <iostream>
#include <memory>
#include "messages_data.h"
#include "archive_index.h"
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/hash.hpp>
#include <boost/thread/condition_variable.hpp>
#include <unordered_map>
#include <codecvt>

const std::wstring PathToDB = L"C:\\Documents and Settings\\V.Babenia\\AppData\\Roaming\\ICQ\\0001\\archive";
std::unordered_map<std::uint64_t, std::uint64_t> p2pChats;

std::vector<std::string> GetLastMessagesProcedure(const std::wstring& uin)
{
	core::archive::history_block messages = {};
	core::archive::headers_list headers = {};
	std::vector<std::string> result;
	auto data = std::make_unique<core::archive::messages_data>(uin + L"\\_db2");
	auto index = std::make_unique<core::archive::archive_index>(uin + L"\\_idx2");
	index->load_from_local();
	auto lastId = index->get_last_msgid();
	index->serialize_from(lastId, 3000, headers);
	data->get_messages(headers, messages);

	auto lastTimestamp = static_cast<std::uint64_t>(messages.back()->get_time());
	auto uintVal = std::uint32_t(0);
	auto uinSubstr = uin.substr(uin.find_last_of(L'\\') + 1, uin.size());
	if (uinSubstr.find(L'@') < uinSubstr.size()) //not quite correct
	{
		boost::hash<std::wstring> str_hash;
		uintVal = static_cast<std::uint32_t>(str_hash(uinSubstr));
	}
	else
	{
		uintVal = std::stoi(uinSubstr);
	}
	auto& currentTimestamp = p2pChats[uintVal];
	if (currentTimestamp < lastTimestamp)
	{
		auto& currentTimestampPos = std::find_if(messages.cbegin(), messages.cend(), 
			[&](const std::shared_ptr<core::archive::history_message>& msg) { return (std::uint64_t)msg->get_time() == currentTimestamp; });
		if (currentTimestampPos != messages.cend())
		{
			++currentTimestampPos;
		}
		for (auto it = currentTimestampPos; it < messages.cend(); ++it)
		{
			auto message = *it;
			auto chatData = message->get_chat_data();
			auto sender = std::string();
			if (chatData)
			{
				sender = chatData->get_sender();
			}
			std::string currentData = "";
			currentData += "Id: ";
			currentData += std::to_string(message->get_msgid());
			currentData += "\n";
			currentData += "Sender: ";
			currentData += (message->get_flags().value_ == 0 ? sender : "Me"); //need to get proper uin
			currentData += "\n";
			currentData += "Time: ";
			currentData += std::to_string(message->get_time());
			currentData += "\n";
			currentData += "Text:\n";
			currentData += message->get_text();
			currentData += "\n";
			result.emplace_back(currentData);
		}
		currentTimestamp = lastTimestamp;
	}
	
	return result;
}

std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

std::vector<std::string> GetAllMessagesProcedure(const std::wstring& uin)
{
	core::archive::history_block messages = {};
	core::archive::headers_list headers = {};
	std::vector<std::string> result;
	auto data = std::make_unique<core::archive::messages_data>(uin + L"\\_db2");
	auto index = std::make_unique<core::archive::archive_index>(uin + L"\\_idx2");
	index->load_from_local();
	auto lastId = index->get_last_msgid();
	index->serialize_from(lastId, 3000, headers);
	data->get_messages(headers, messages);

	for (auto& message : messages)
	{
		auto chatData = message->get_chat_data();
		auto sender = std::string();
		if (chatData)
		{
			sender = chatData->get_sender();
		}
		if (sender.empty())
		{
			auto uinSubstr = uin.substr(uin.find_last_of(L'\\') + 1, uin.size());
			sender = std::move(wstring_to_utf8(uinSubstr));
		}
        auto senderHandle = [&] {
            const auto& flags = message->get_flags().flags_;
            auto deletedStatus = flags.deleted_ & 1 ? " (deleted)" : "";
            if (flags.outgoing_ & 1)
            {
                return std::string("me") + deletedStatus;
            }
            else
            {
                return sender + deletedStatus;
            }
        };

        auto messageHandle = [&] {
            const auto& fileSharingData = message->get_file_sharing_data();
            if (fileSharingData)
            {
                return std::string(std::string("Id: ") + std::to_string(message->get_msgid()) + "\nSender (uin): " + senderHandle() + "\nFile transferring: " + message->get_text() + "\n");
            }
            std::string currentData = "";
            currentData += "Id: ";
            currentData += std::to_string(message->get_msgid());
            currentData += "\n";
            currentData += "Sender (uin): ";
            currentData += senderHandle();
            currentData += "\n";
            currentData += "Time: ";
            currentData += std::to_string(message->get_time());
            currentData += "\n";
            currentData += "Text:\n";
            currentData += message->get_text();
            currentData += "\n";
            return currentData;
        };

		result.emplace_back(messageHandle());
	}
	return result;
}

void MainRoutine(const std::vector<std::wstring>& directories)
{
	while (true)
	{
		system("cls");
		for (auto& directory : directories)
		{
			auto&& messages = GetLastMessagesProcedure(directory.c_str());
			if (!messages.empty())
			{
				for (const auto& message : messages)
				{
					std::cout << message << "\n";
				}
			}
		}
		boost::this_thread::sleep_for(boost::chrono::seconds(10));
	}
}

int main()
{
	//std::vector<std::wstring> directories;
	//namespace fs = boost::filesystem;
	//fs::path targetDir(PathToDB);
	//fs::directory_iterator it(targetDir), eod;
	//BOOST_FOREACH(fs::path const &p, std::make_pair(it, eod))
	//{
	//	if (fs::is_directory(p))
	//	{
	//		directories.emplace_back(p.c_str());
	//	}
	//}

	//auto routineThread = boost::thread(&MainRoutine, directories);
	//routineThread.join();

	auto&& messages = GetAllMessagesProcedure(L"C:\\Documents and Settings\\V.Babenia\\AppData\\Roaming\\ICQ\\0001\\archive\\4263608");
	for (auto& message : messages)
	{
		std::cout << message << "\n";
	}

	system("pause");
	return 0;
}