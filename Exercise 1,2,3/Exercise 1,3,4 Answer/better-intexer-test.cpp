
#include <gtest/gtest.h>

extern "C"
{
#include "better-intexer.c"
}

#define DATA_PATH "DATA_PATH"

TEST(handle_path, test_input)
{
	char full_path[MAX_PATH] = {'x', 'x', 'x'};
	
	ASSERT_EQ(0, unsetenv(DATA_PATH));

	ASSERT_EQ(0, handle_path_arg(sizeof(full_path), full_path, NULL));
	ASSERT_STREQ("", full_path);

	ASSERT_EQ(1, handle_path_arg(sizeof(full_path), full_path, "a"));
	ASSERT_STREQ("a", full_path);

	ASSERT_EQ(0, handle_path_arg(sizeof(full_path), full_path, ""));
	ASSERT_STREQ("", full_path);

	ASSERT_EQ(0, setenv(DATA_PATH, "", 1));
	ASSERT_EQ(0, handle_path_arg(sizeof(full_path), full_path, NULL));
	ASSERT_STREQ("", full_path);

	ASSERT_EQ(0, setenv(DATA_PATH, "", 1));
	ASSERT_EQ(0, handle_path_arg(sizeof(full_path), full_path, ""));
	ASSERT_STREQ("", full_path);
}

TEST(handle_path, test_large_input)
{
	char large_input[MAX_PATH+10];
	char full_path[MAX_PATH];

	size_t i = 0;

	for(i=0; i < sizeof(large_input) - 1; i++)
	{
		large_input[i] = 'a';
	}

	large_input[i] = '\0';

	ASSERT_EQ(0, unsetenv(DATA_PATH));

	ASSERT_EQ(sizeof(large_input) - 1, handle_path_arg(sizeof(full_path), full_path, large_input));
}

TEST(handle_path, test_sep)
{
	char full_path[MAX_PATH];
	const char *data_path = "abc";
	const char *data_path_sep = "abc/";
	const char *path_arg = "xyz";

	ASSERT_EQ(0, setenv(DATA_PATH, data_path_sep, 1));

	ASSERT_EQ(strlen(data_path_sep), handle_path_arg(sizeof(full_path), full_path, NULL));
	ASSERT_STREQ(data_path_sep, full_path);

	ASSERT_EQ(strlen(data_path_sep) + strlen(path_arg), handle_path_arg(sizeof(full_path), full_path, path_arg));
	ASSERT_STREQ("abc/xyz", full_path);

	ASSERT_EQ(0, setenv(DATA_PATH, data_path, 1));

	ASSERT_EQ(strlen(data_path_sep), handle_path_arg(sizeof(full_path), full_path, NULL));
	ASSERT_STREQ(data_path_sep, full_path);

	ASSERT_EQ(strlen(data_path_sep) + strlen(path_arg), handle_path_arg(sizeof(full_path), full_path, path_arg));
	ASSERT_STREQ("abc/xyz", full_path);
}

static FILE *writestr( const char *str )
{
	FILE *fh = fopen("test.tmp", "w");
	if( fh )
	{
		if( EOF != fputs(str, fh) )
		{
			fclose(fh);
			fh = NULL;
		}

		if( NULL != fh )
		{
			fclose(fh);
			fh = NULL;
		}
		else
		{
			fh = fopen("test.tmp", "r");
		}
	}
	return fh;
}

struct test_db {
	bool success;
	const char *db;
	size_t exp_size;
	struct sigrecord *exp_records;
};

std::ostream &operator<<(std::ostream& stream, const struct sigrecord &record )
{
	stream << "id: " << record.signum << std::endl
		<< "name: \"" << record.signame << "\"" << std::endl
		<< "desc: \"" << record.sigdesc << "\"" << std::endl;
	return stream;
}

std::ostream &operator<<(std::ostream& stream, const test_db &record )
{
	stream << "success: " << record.success << std::endl;
	return stream;
}

bool operator==(const struct sigrecord &lhs, const struct sigrecord &rhs)
{
	return lhs.signum == rhs.signum
		&& 0 == strcmp( lhs.signame, rhs.signame )
		&& 0 == strcmp( lhs.sigdesc, rhs.sigdesc )
		;
}

class DbTestFixture : public ::testing::TestWithParam<test_db>
{
};

TEST_P(DbTestFixture, test_db_values)
{
	test_db data = GetParam();

	struct sigrecord *records = NULL;
	size_t size = 0;
	FILE *fh = writestr(data.db);

	ASSERT_TRUE( NULL != fh );

 	records = read_records(fh, &size);
	if( data.success )
	{
		ASSERT_TRUE( NULL != records );
		ASSERT_EQ(data.exp_size, size);

		for( size_t i = 0; i < data.exp_size; i++ )
			EXPECT_EQ( data.exp_records[i], records[i] );
	}
	else
	{
		ASSERT_TRUE( NULL == records );
	}

	free(records);
	fclose(fh);
}
struct sigrecord onerec{1, "ABC", "DESC"};
struct sigrecord onebadrec{1, "A", ""};

struct sigrecord tworec[2] = {
	{1, "ABC", "DESC"},
	{2, "XYZ", "BLAH"}
};

struct sigrecord name1[2] = {
	{1, "01234", "DESC"},
	{2, "XYZ", "BLAH"}
};

struct sigrecord name2[2] = {
	{1, "012345", "DESC"},
	{2, "XYZ", "BLAH"}
};

struct sigrecord name3[2] = {
	{1, "012345", "DESC"},
	{2, "XYZ", "BLAH"}
};

struct sigrecord desc1[2] = {
	{1, "012345", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"},
	{2, "XYZ", "BLAH"}
};

struct sigrecord desc2[2] = {
	{1, "012345", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"},
	{2, "XYZ", "BLAH"}
};

INSTANTIATE_TEST_CASE_P(InstantiationName,
                        DbTestFixture,
                        ::testing::Values(
				test_db{true,  "1\n1 ABC DESC\n", 1, &onerec},
				test_db{true,  "1\n1 A ", 1, &onebadrec},
				test_db{true,  "1 \n1 A \n", 1, &onebadrec},
				test_db{true,  "2\n1 ABC DESC\n2 XYZ BLAH\n", 2, tworec},
				test_db{true,  "1\n1 ABC DESC\n2 XYZ BLAH\n", 1, &tworec[0]},
				test_db{true,  "2\n1 01234 DESC\n2 XYZ BLAH\n", 2, name1},
				test_db{true,  "2\n1 012345 DESC\n2 XYZ BLAH\n", 2, name2},
				test_db{false, "2\n1 0123456 DESC\n2 XYZ BLAH\n", 2, name3},
				test_db{true,  "2\n1 012345 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n2 XYZ BLAH\n", 2, desc1},
				test_db{true,  "2\n1 012345 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n2 XYZ BLAH\n", 2, desc2},
				test_db{false, "2\n1 012345 xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n2 XYZ BLAH\n", 2, desc1},
				test_db{false, "1 \n1  \n", 1, &onebadrec},
				test_db{false, "1\n1  DESC\n", 1, &onebadrec},
				test_db{false, "1", 1, &onebadrec},
				test_db{false, "1\n", 1, &onebadrec},
				test_db{false, "1\n1", 1, &onebadrec},
				test_db{false, "1\n1 ", 1, &onebadrec},
				test_db{false, "1\n1 A", 1, &onebadrec},
				test_db{false, "2\n1 A \n", 1, &onebadrec},
				test_db{false, "-1\n1 A \n", 1, &onebadrec},
				test_db{false, "0\n1 A \n", 0, &onebadrec},
				test_db{false, "100000000000000000000000000000000000000000000000\n1 A \n", 1, &onebadrec}
				));
