#include <git2.h>
#include <string>
#include <stdexcept>
#include "git.h"

using namespace std;

class git_exception : public exception {
public:
	git_exception(int error, string_view func) {
		auto lg2err = git_error_last();

		if (lg2err && lg2err->message)
			msg = string(func) + " failed (" + lg2err->message + ")";
		else
			msg = string(func) + " failed (error " + to_string(error) + ")";
	}

	const char* what() const noexcept {
		return msg.c_str();
	}

private:
	string msg;
};

GitSignature::GitSignature(const string& user, const string& email) {
	if (auto ret = git_signature_now(&sig, user.c_str(), email.c_str()))
		throw git_exception(ret, "git_signature_now");
}

GitSignature::GitSignature(const string& user, const string& email, time_t dt, signed int offset) {
	if (auto ret = git_signature_new(&sig, user.c_str(), email.c_str(), dt, offset))
		throw git_exception(ret, "git_signature_new");
}

GitSignature::~GitSignature() {
	git_signature_free(sig);
}

GitTree::GitTree(const git_commit* commit) {
	if (auto ret = git_commit_tree(&tree, commit))
		throw git_exception(ret, "git_commit_tree");
}

GitTree::GitTree(const GitRepo& repo, const git_oid& oid) {
	if (auto ret = git_tree_lookup(&tree, repo, &oid))
		throw git_exception(ret, "git_tree_lookup");
}

GitTree::GitTree(const GitRepo& repo, const GitTreeEntry& gte) {
	if (auto ret = git_tree_entry_to_object((git_object**)&tree, repo, gte))
		throw git_exception(ret, "git_tree_entry_to_object");
}

GitTree::~GitTree() {
	git_tree_free(tree);
}

bool GitTree::entry_bypath(git_tree_entry** out, const string& path) {
	int ret = git_tree_entry_bypath(out, tree, path.c_str());

	if (ret != 0 && ret != GIT_ENOTFOUND)
		throw git_exception(ret, "git_tree_entry_bypath");

	return ret == 0;
}

GitRepo::GitRepo(const string& dir) {
	if (auto ret = git_repository_open(&repo, dir.c_str()))
		throw git_exception(ret, "git_repository_open");
}

GitRepo::~GitRepo() {
	git_repository_free(repo);
}

void GitRepo::reference_name_to_id(git_oid* out, const string& name) {
	if (auto ret = git_reference_name_to_id(out, repo, name.c_str()))
		throw git_exception(ret, "git_reference_name_to_id");
}

void GitRepo::commit_lookup(git_commit** commit, const git_oid* oid) {
	if (auto ret = git_commit_lookup(commit, repo, oid))
		throw git_exception(ret, "git_commit_lookup");
}

git_oid GitRepo::commit_create(const string& update_ref, const GitSignature& author, const GitSignature& committer, const string& message, const GitTree& tree, git_commit* parent) {
	git_oid id;

	if (auto ret = git_commit_create_v(&id, repo, update_ref.c_str(), author, committer, nullptr, message.c_str(), tree, 1, parent))
		throw git_exception(ret, "git_commit_create_v");

	return id;
}

git_oid GitRepo::blob_create_frombuffer(const string& data) {
	git_oid blob;

	if (auto ret = git_blob_create_frombuffer(&blob, repo, data.c_str(), data.length()))
		throw git_exception(ret, "git_blob_create_frombuffer");

	return blob;
}

git_oid GitRepo::tree_create_updated(const GitTree& baseline, size_t nupdates, const git_tree_update* updates) {
	git_oid oid;

	if (auto ret = git_tree_create_updated(&oid, repo, baseline, nupdates, updates))
		throw git_exception(ret, "git_tree_create_updated");

	return oid;
}

GitDiff::GitDiff(const GitRepo& repo, const GitTree& old_tree, const GitTree& new_tree, const git_diff_options* opts) {
	if (auto ret = git_diff_tree_to_tree(&diff, repo, old_tree, new_tree, opts))
		throw git_exception(ret, "git_diff_tree_to_tree");
}

GitDiff::~GitDiff() {
	git_diff_free(diff);
}

size_t GitDiff::num_deltas() {
	return git_diff_num_deltas(diff);
}

GitIndex::GitIndex(const GitRepo& repo) {
	if (auto ret = git_repository_index(&index, repo))
		throw git_exception(ret, "git_repository_index");
}

GitIndex::~GitIndex() {
	git_index_free(index);
}

void GitIndex::write_tree(git_oid* oid) {
	if (auto ret = git_index_write_tree(oid, index))
		throw git_exception(ret, "git_index_write_tree");
}

void GitIndex::add_bypath(const string& fn) {
	if (auto ret = git_index_add_bypath(index, fn.c_str()))
		throw git_exception(ret, "git_index_add_bypath");
}

void GitIndex::remove_bypath(const string& fn) {
	if (auto ret = git_index_remove_bypath(index, fn.c_str()))
		throw git_exception(ret, "git_index_remove_bypath");
}

void GitIndex::clear() {
	if (auto ret = git_index_clear(index))
		throw git_exception(ret, "git_index_clear");
}

size_t GitTree::entrycount() {
	return git_tree_entrycount(tree);
}

GitTreeEntry::GitTreeEntry(GitTree& tree, size_t idx) {
	gte = git_tree_entry_byindex(tree, idx);

	if (!gte)
		throw runtime_error("git_tree_entry_byindex returned NULL.");
}

string GitTreeEntry::name() {
	return git_tree_entry_name(gte);
}

git_otype GitTreeEntry::type() {
	return git_tree_entry_type(gte);
}

GitTree::GitTree(const GitRepo& repo, const string& rev) {
	if (auto ret = git_revparse_single((git_object**)&tree, repo, rev.c_str()))
		throw git_exception(ret, "git_revparse_single");
}

GitBlob::GitBlob(const GitTree& tree, const string& path) {
	if (auto ret = git_object_lookup_bypath(&obj, (git_object*)(git_tree*)tree, path.c_str(), GIT_OBJ_BLOB))
		throw git_exception(ret, "git_object_lookup_bypath");
}

GitBlob::~GitBlob() {
	git_object_free(obj);
}

GitBlob::operator string() const {
	return string((char*)git_blob_rawcontent((git_blob*)obj), git_blob_rawsize((git_blob*)obj));
}
