#pragma once

namespace core
{
	namespace archive
	{
		enum error
		{
			ok,
			end_of_file,
			file_not_exist,
			create_directory_error,
			open_file_error
		};
	}
}