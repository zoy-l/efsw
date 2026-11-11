#include "test_util.hpp"
#include "utest.h"
#include <efsw/FileInfo.hpp>
#include <efsw/FileSystem.hpp>
#include <filesystem>
#include <string>

using namespace efsw_test;

UTEST( FileSystem, EmptyPathsStayEmpty ) {
	std::string path;
	efsw::FileSystem::dirAddSlashAtEnd( path );
	EXPECT_TRUE( path.empty() );
	efsw::FileSystem::dirRemoveSlashAtEnd( path );
	EXPECT_TRUE( path.empty() );
	EXPECT_FALSE( efsw::FileSystem::slashAtEnd( path ) );
	EXPECT_TRUE( efsw::FileSystem::fileNameFromPath( path ).empty() );
	EXPECT_TRUE( efsw::FileSystem::pathRemoveFileName( path ).empty() );
}

UTEST( FileSystem, SingleCharacterRelativePathsKeepTheirMeaning ) {
	const char slash = efsw::FileSystem::getOSSlash();
	std::string current( "." );
	const std::string currentWithSlash = current + slash;
	efsw::FileSystem::dirAddSlashAtEnd( current );
	EXPECT_STREQ( currentWithSlash.c_str(), current.c_str() );
	efsw::FileSystem::dirRemoveSlashAtEnd( current );
	EXPECT_STREQ( ".", current.c_str() );

	std::string directory( "a" );
	const std::string directoryWithSlash = directory + slash;
	efsw::FileSystem::dirAddSlashAtEnd( directory );
	EXPECT_STREQ( directoryWithSlash.c_str(), directory.c_str() );
	efsw::FileSystem::dirRemoveSlashAtEnd( directory );
	EXPECT_STREQ( "a", directory.c_str() );
}

UTEST( FileSystem, NonRootDirectorySeparatorsCanBeRemoved ) {
	const char slash = efsw::FileSystem::getOSSlash();
	const std::string parent = std::string( "parent" ) + slash;
	std::string path = parent + "child" + slash;
	efsw::FileSystem::dirRemoveSlashAtEnd( path );
	EXPECT_TRUE( path == parent + "child" );
	EXPECT_TRUE( efsw::FileSystem::pathRemoveFileName( path ) == parent );
	EXPECT_TRUE( efsw::FileSystem::fileNameFromPath( path ) == "child" );
}

UTEST( FileSystem, CurrentDirectoryInfoKeepsItsPath ) {
	const std::string path = std::string( "." ) + efsw::FileSystem::getOSSlash();
	efsw::FileInfo info( path );
	EXPECT_TRUE( info.isDirectory() );
	EXPECT_TRUE( info.exists() );
	info.getRealInfo();
	EXPECT_TRUE( info.isDirectory() );
	EXPECT_STREQ( path.c_str(), info.Filepath.c_str() );
}

UTEST( FileSystem, RootFileInfoKeepsItsIdentityAndPath ) {
	const std::string root = std::filesystem::current_path().root_path().u8string();
	const efsw::FileInfo expected( root );
	efsw::FileInfo info( root, true );
	EXPECT_TRUE( info.isDirectory() );
	EXPECT_TRUE( info.exists() );
	EXPECT_TRUE( efsw::FileInfo::exists( root ) );
	EXPECT_EQ( expected.Device, info.Device );
	EXPECT_EQ( expected.Inode, info.Inode );
	EXPECT_STREQ( root.c_str(), info.Filepath.c_str() );
	info.getInfo();
	info.getInfo();
	EXPECT_TRUE( info.isDirectory() );
	EXPECT_EQ( expected.Device, info.Device );
	EXPECT_EQ( expected.Inode, info.Inode );
	EXPECT_STREQ( root.c_str(), info.Filepath.c_str() );
}

UTEST( FileSystem, RootCanBeSubscribedAndRemovedByPath ) {
	const std::string root = std::filesystem::current_path().root_path().u8string();
	TestListener listener;
	efsw::FileWatcher watcher( useGeneric, 100 );
	watcher.watch();
	const efsw::WatchID id = watcher.addWatch( root, &listener, false );
	ASSERT_GT( id, 0 );
	const auto directories = watcher.directories();
	ASSERT_EQ( 1u, directories.size() );
	EXPECT_STREQ( root.c_str(), directories.front().c_str() );
	watcher.removeWatch( root );
	EXPECT_TRUE( watcher.directories().empty() );
	const efsw::WatchID replacement = watcher.addWatch( root, &listener, false );
	ASSERT_GT( replacement, 0 );
	watcher.removeWatch( replacement );
	EXPECT_TRUE( watcher.directories().empty() );
}

#if EFSW_OS == EFSW_OS_WIN

UTEST( FileSystem, WindowsRootsKeepRequiredSeparators ) {
	const std::string roots[] = { "\\", "C:\\", "\\\\?\\C:\\", "\\\\.\\C:\\",
								  "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\" };
	for ( const auto& root : roots ) {
		std::string path = root;
		efsw::FileSystem::dirRemoveSlashAtEnd( path );
		EXPECT_STREQ( root.c_str(), path.c_str() );
		efsw::FileSystem::dirAddSlashAtEnd( path );
		EXPECT_STREQ( root.c_str(), path.c_str() );
		EXPECT_TRUE( efsw::FileSystem::fileNameFromPath( path ).empty() );
		EXPECT_TRUE( efsw::FileSystem::pathRemoveFileName( path ) == root );

		std::string child = root + "child\\";
		efsw::FileSystem::dirRemoveSlashAtEnd( child );
		EXPECT_TRUE( child == root + "child" );
		EXPECT_TRUE( efsw::FileSystem::pathRemoveFileName( child ) == root );
	}
}

UTEST( FileSystem, WindowsForwardSlashRootIsNotMadeDriveRelative ) {
	std::string path( "C:/" );
	efsw::FileSystem::dirRemoveSlashAtEnd( path );
	EXPECT_STREQ( "C:/", path.c_str() );
	path += '\\';
	efsw::FileSystem::dirRemoveSlashAtEnd( path );
	EXPECT_STREQ( "C:/", path.c_str() );
}

UTEST( FileSystem, UncShareSeparatorsCanBeRemoved ) {
	const std::string shares[] = { "\\\\server\\share", "\\\\?\\UNC\\server\\share" };
	for ( const auto& share : shares ) {
		std::string path = share + '\\';
		efsw::FileSystem::dirRemoveSlashAtEnd( path );
		EXPECT_STREQ( share.c_str(), path.c_str() );
		efsw::FileSystem::dirAddSlashAtEnd( path );
		EXPECT_TRUE( path == share + '\\' );
	}
}

#else

UTEST( FileSystem, PosixRootsKeepTheirSeparators ) {
	const std::string roots[] = { "/", "//", "///" };
	for ( const auto& root : roots ) {
		std::string path = root;
		efsw::FileSystem::dirRemoveSlashAtEnd( path );
		EXPECT_STREQ( root.c_str(), path.c_str() );
		EXPECT_TRUE( efsw::FileSystem::fileNameFromPath( path ).empty() );
		EXPECT_TRUE( efsw::FileSystem::pathRemoveFileName( path ) == root );
	}
}

UTEST( FileSystem, PosixBackslashesRemainFileNameCharacters ) {
	std::string path( "parent/name\\" );
	EXPECT_FALSE( efsw::FileSystem::slashAtEnd( path ) );
	efsw::FileSystem::dirRemoveSlashAtEnd( path );
	EXPECT_STREQ( "parent/name\\", path.c_str() );
	EXPECT_TRUE( efsw::FileSystem::fileNameFromPath( path ) == "name\\" );
	EXPECT_TRUE( efsw::FileSystem::pathRemoveFileName( path ) == "parent/" );
}

#endif
