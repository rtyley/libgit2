/*
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * In addition to the permissions in the GNU General Public License,
 * the authors give you unlimited permission to link the compiled
 * version of this file into combinations with other programs,
 * and to distribute those combinations without any restriction
 * coming from the use of this file.  (The General Public License
 * restrictions do apply in other respects; for example, they cover
 * modification of the file, and distribution when not linked into
 * a combined executable.)
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "test_lib.h"
#include "test_helpers.h"
#include "fileops.h"
#include "git2/status.h"

static const char *test_blob_oid = "d4fa8600b4f37d7516bef4816ae2c64dbf029e3a";

#define STATUS_WORKDIR_FOLDER TEST_RESOURCES "/status/"
#define STATUS_REPOSITORY_TEMP_FOLDER TEMP_REPO_FOLDER ".gitted/"

static int file_create(const char *filename, const char *content)
{
	int fd;

	fd = p_creat(filename, 0644);
	if (fd == 0)
		return GIT_ERROR;
	if (p_write(fd, content, strlen(content)) != 0)
		return GIT_ERROR;
	if (p_close(fd) != 0)
		return GIT_ERROR;

	return GIT_SUCCESS;
}

BEGIN_TEST(file0, "test retrieving OID from a file apart from the ODB")
	git_oid expected_id, actual_id;
	char filename[] = "new_file";

	must_pass(file_create(filename, "new_file\n\0"));

	must_pass(git_odb_hashfile(&actual_id, filename, GIT_OBJ_BLOB));

	must_pass(git_oid_fromstr(&expected_id, test_blob_oid));
	must_be_true(git_oid_cmp(&expected_id, &actual_id) == 0);

	must_pass(p_unlink(filename));
END_TEST

static const char *entry_paths0[] = {
	"file_deleted",
	"modified_file",
	"new_file",
	"staged_changes",
	"staged_changes_file_deleted",
	"staged_changes_modified_file",
	"staged_delete_file_deleted",
	"staged_delete_modified_file",
	"staged_new_file",
	"staged_new_file_deleted_file",
	"staged_new_file_modified_file",

	"subdir/deleted_file",
	"subdir/modified_file",
	"subdir/new_file",
};

static const unsigned int entry_statuses0[] = {
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_MODIFIED,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_DELETED,
	GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED,
	GIT_STATUS_INDEX_DELETED,
	GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_NEW,
	GIT_STATUS_INDEX_NEW,
	GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_DELETED,
	GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_MODIFIED,

	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_MODIFIED,
	GIT_STATUS_WT_NEW,
};

#define ENTRY_COUNT0 14

struct status_entry_counts {
	int wrong_status_flags_count;
	int wrong_sorted_path;
	int entry_count;
	const unsigned int* expected_statuses;
	const char** expected_paths;
	int expected_entry_count;
};

static int status_cb(const char *path, unsigned int status_flags, void *payload)
{
	struct status_entry_counts *counts = (struct status_entry_counts *)payload;

	if (counts->entry_count >= counts->expected_entry_count) {
		counts->wrong_status_flags_count++;
		goto exit;
	}

	if (strcmp(path, counts->expected_paths[counts->entry_count])) {
		counts->wrong_sorted_path++;
		goto exit;
	}

	if (status_flags != counts->expected_statuses[counts->entry_count])
		counts->wrong_status_flags_count++;

exit:
	counts->entry_count++;
	return GIT_SUCCESS;
}

BEGIN_TEST(statuscb0, "test retrieving status for worktree of repository")
	git_repository *repo;
	struct status_entry_counts counts;

	must_pass(copydir_recurs(STATUS_WORKDIR_FOLDER, TEMP_REPO_FOLDER));
	must_pass(git_futils_mv_atomic(STATUS_REPOSITORY_TEMP_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	memset(&counts, 0x0, sizeof(struct status_entry_counts));
	counts.expected_entry_count = ENTRY_COUNT0;
	counts.expected_paths = entry_paths0;
	counts.expected_statuses = entry_statuses0;

	must_pass(git_status_foreach(repo, status_cb, &counts));
	must_be_true(counts.entry_count == counts.expected_entry_count);
	must_be_true(counts.wrong_status_flags_count == 0);
	must_be_true(counts.wrong_sorted_path == 0);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

static int status_cb1(const char *GIT_UNUSED(path), unsigned int GIT_UNUSED(status_flags), void *payload)
{
	int *count = (int *)payload;;

	GIT_UNUSED_ARG(path);
	GIT_UNUSED_ARG(status_flags);

	(void) *count++;

	return GIT_SUCCESS;
}

BEGIN_TEST(statuscb1, "test retrieving status for a worktree of an empty repository")
	git_repository *repo;
	int count = 0;

	must_pass(copydir_recurs(EMPTY_REPOSITORY_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(remove_placeholders(TEST_STD_REPO_FOLDER, "dummy-marker.txt"));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	must_pass(git_status_foreach(repo, status_cb1, &count));
	must_be_true(count == 0);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

static const char *entry_paths2[] = {
	"current_file",
	"file_deleted",
	"modified_file",
	"staged_changes",
	"staged_changes_file_deleted",
	"staged_changes_modified_file",
	"staged_delete_file_deleted",
	"staged_delete_modified_file",
	"staged_new_file",
	"staged_new_file_deleted_file",
	"staged_new_file_modified_file",
	"subdir/current_file",
	"subdir/deleted_file",
	"subdir/modified_file",
};

static const unsigned int entry_statuses2[] = {
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_INDEX_DELETED,
	GIT_STATUS_INDEX_DELETED,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_NEW,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_NEW,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_NEW,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED,
};

#define ENTRY_COUNT2 14

BEGIN_TEST(statuscb2, "test retrieving status for a purged worktree of an valid repository")
	git_repository *repo;
	struct status_entry_counts counts;

	must_pass(copydir_recurs(STATUS_WORKDIR_FOLDER, TEMP_REPO_FOLDER));
	must_pass(git_futils_mv_atomic(STATUS_REPOSITORY_TEMP_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	/* Purging the working */
	must_pass(p_unlink(TEMP_REPO_FOLDER "current_file"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "modified_file"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "new_file"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "staged_changes"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "staged_changes_modified_file"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "staged_delete_modified_file"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "staged_new_file"));
	must_pass(p_unlink(TEMP_REPO_FOLDER "staged_new_file_modified_file"));
	must_pass(git_futils_rmdir_r(TEMP_REPO_FOLDER "subdir", 1));

	memset(&counts, 0x0, sizeof(struct status_entry_counts));
	counts.expected_entry_count = ENTRY_COUNT2;
	counts.expected_paths = entry_paths2;
	counts.expected_statuses = entry_statuses2;

	must_pass(git_status_foreach(repo, status_cb, &counts));
	must_be_true(counts.entry_count == counts.expected_entry_count);
	must_be_true(counts.wrong_status_flags_count == 0);
	must_be_true(counts.wrong_sorted_path == 0);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

static const char *entry_paths3[] = {
	".HEADER",
	"42-is-not-prime.sigh",
	"README.md",
	"current_file",
	"current_file/current_file",
	"current_file/modified_file",
	"current_file/new_file",
	"file_deleted",
	"modified_file",
	"new_file",
	"staged_changes",
	"staged_changes_file_deleted",
	"staged_changes_modified_file",
	"staged_delete_file_deleted",
	"staged_delete_modified_file",
	"staged_new_file",
	"staged_new_file_deleted_file",
	"staged_new_file_modified_file",
	"subdir",
	"subdir/current_file",
	"subdir/deleted_file",
	"subdir/modified_file",
};

static const unsigned int entry_statuses3[] = {
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_MODIFIED,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_WT_MODIFIED | GIT_STATUS_INDEX_MODIFIED,
	GIT_STATUS_INDEX_DELETED,
	GIT_STATUS_WT_NEW | GIT_STATUS_INDEX_DELETED,
	GIT_STATUS_INDEX_NEW,
	GIT_STATUS_WT_DELETED | GIT_STATUS_INDEX_NEW,
	GIT_STATUS_WT_MODIFIED | GIT_STATUS_INDEX_NEW,
	GIT_STATUS_WT_NEW,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED,
	GIT_STATUS_WT_DELETED,
};

#define ENTRY_COUNT3 22

BEGIN_TEST(statuscb3, "test retrieving status for a worktree where a file and a subdir have been renamed and some files have been added")
	git_repository *repo;
	struct status_entry_counts counts;

	must_pass(copydir_recurs(STATUS_WORKDIR_FOLDER, TEMP_REPO_FOLDER));
	must_pass(git_futils_mv_atomic(STATUS_REPOSITORY_TEMP_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	must_pass(git_futils_mv_atomic(TEMP_REPO_FOLDER "current_file", TEMP_REPO_FOLDER "swap"));
	must_pass(git_futils_mv_atomic(TEMP_REPO_FOLDER "subdir", TEMP_REPO_FOLDER "current_file"));
	must_pass(git_futils_mv_atomic(TEMP_REPO_FOLDER "swap", TEMP_REPO_FOLDER "subdir"));

	must_pass(file_create(TEMP_REPO_FOLDER ".HEADER", "dummy"));
	must_pass(file_create(TEMP_REPO_FOLDER "42-is-not-prime.sigh", "dummy"));
	must_pass(file_create(TEMP_REPO_FOLDER "README.md", "dummy"));

	memset(&counts, 0x0, sizeof(struct status_entry_counts));
	counts.expected_entry_count = ENTRY_COUNT3;
	counts.expected_paths = entry_paths3;
	counts.expected_statuses = entry_statuses3;

	must_pass(git_status_foreach(repo, status_cb, &counts));
	must_be_true(counts.entry_count == counts.expected_entry_count);
	must_be_true(counts.wrong_status_flags_count == 0);
	must_be_true(counts.wrong_sorted_path == 0);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

BEGIN_TEST(singlestatus0, "test retrieving status for single file")
	git_repository *repo;
	unsigned int status_flags;
	int i;

	must_pass(copydir_recurs(STATUS_WORKDIR_FOLDER, TEMP_REPO_FOLDER));
	must_pass(git_futils_mv_atomic(STATUS_REPOSITORY_TEMP_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	for (i = 0; i < ENTRY_COUNT0; ++i) {
		must_pass(git_status_file(&status_flags, repo, entry_paths0[i]));
		must_be_true(status_flags == entry_statuses0[i]);
	}

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

BEGIN_TEST(singlestatus1, "test retrieving status for nonexistent file")
	git_repository *repo;
	unsigned int status_flags;
	int error;

	must_pass(copydir_recurs(STATUS_WORKDIR_FOLDER, TEMP_REPO_FOLDER));
	must_pass(git_futils_mv_atomic(STATUS_REPOSITORY_TEMP_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	// "nonexistent" does not exist in HEAD, Index or the worktree
	error = git_status_file(&status_flags, repo, "nonexistent");
	must_be_true(error == GIT_ENOTFOUND);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

BEGIN_TEST(singlestatus2, "test retrieving status for a non existent file in an empty repository")
	git_repository *repo;
	unsigned int status_flags;
	int error;

	must_pass(copydir_recurs(EMPTY_REPOSITORY_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(remove_placeholders(TEST_STD_REPO_FOLDER, "dummy-marker.txt"));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	error = git_status_file(&status_flags, repo, "nonexistent");
	must_be_true(error == GIT_ENOTFOUND);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

BEGIN_TEST(singlestatus3, "test retrieving status for a new file in an empty repository")
	git_repository *repo;
	unsigned int status_flags;
	char file_path[GIT_PATH_MAX];
	char filename[] = "new_file";
	int fd;

	must_pass(copydir_recurs(EMPTY_REPOSITORY_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(remove_placeholders(TEST_STD_REPO_FOLDER, "dummy-marker.txt"));

	git_path_join(file_path, TEMP_REPO_FOLDER, filename);
	fd = p_creat(file_path, 0644);
	must_pass(fd);
	must_pass(p_write(fd, "new_file\n", 9));
	must_pass(p_close(fd));

	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	must_pass(git_status_file(&status_flags, repo, filename));
	must_be_true(status_flags == GIT_STATUS_WT_NEW);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

BEGIN_TEST(singlestatus4, "can't determine the status for a folder")
	git_repository *repo;
	unsigned int status_flags;
	int error;

	must_pass(copydir_recurs(STATUS_WORKDIR_FOLDER, TEMP_REPO_FOLDER));
	must_pass(git_futils_mv_atomic(STATUS_REPOSITORY_TEMP_FOLDER, TEST_STD_REPO_FOLDER));
	must_pass(git_repository_open(&repo, TEST_STD_REPO_FOLDER));

	error = git_status_file(&status_flags, repo, "subdir");
	must_be_true(error == GIT_EINVALIDPATH);

	git_repository_free(repo);

	git_futils_rmdir_r(TEMP_REPO_FOLDER, 1);
END_TEST

BEGIN_SUITE(status)
	ADD_TEST(file0);

	ADD_TEST(statuscb0);
	ADD_TEST(statuscb1);
	ADD_TEST(statuscb2);
	ADD_TEST(statuscb3);

	ADD_TEST(singlestatus0);
	ADD_TEST(singlestatus1);
	ADD_TEST(singlestatus2);
	ADD_TEST(singlestatus3);
	ADD_TEST(singlestatus4);
END_SUITE
