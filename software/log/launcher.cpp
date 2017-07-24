#include "log/launcher.h"
#include "log/analyzer.h"
#include "log/loader.h"
#include "log/player.h"
#include "util/algorithm.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/mapped_file.h"
#include "util/string.h"
#include <bzlib.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>
#include <giomm/file.h>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeselection.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	std::string get_logs_dir() {
		const std::string &parent_dir = Glib::get_user_data_dir();
		const std::string &tbots_dir = Glib::build_filename(parent_dir, "thunderbots");
		return Glib::build_filename(tbots_dir, "logs");
	}

	std::string filename_to_pathname(const std::string &filename) {
		return Glib::build_filename(get_logs_dir(), filename);
	}

	void get_filenames(std::vector<std::string> &files) {
		Glib::Dir dir(get_logs_dir());
		files.clear();
		std::copy_if(dir.begin(), dir.end(), std::back_inserter(files), [](const std::string &s) { return !s.empty() && s[0] != '.' && LogLoader::is_current_version(filename_to_pathname(s)); });
		std::sort(files.begin(), files.end());
		std::reverse(files.begin(), files.end());
	}
}

LogLauncher::LogLauncher() : log_list(1, false, Gtk::SELECTION_MULTIPLE), analyzer_button(u8"Analyzer"), player_button(u8"Player"), rename_button(u8"Rename"), delete_button(u8"Delete"), export_button(u8"Export"), import_button(u8"Import"), exit_pending(false) {
	set_title(u8"Thunderbots Log Tools");
	set_size_request(400, 400);

	populate();

	log_list.set_column_title(0, u8"Log File");
	scroller.add(log_list);
	hbox.pack_start(scroller, Gtk::PACK_EXPAND_WIDGET);

	vbb.pack_start(analyzer_button);
	vbb.pack_start(player_button);
	vbb.pack_start(rename_button);
	vbb.pack_start(delete_button);
	vbb.pack_start(export_button);
	vbb.pack_start(import_button);
	hbox.pack_start(vbb, Gtk::PACK_SHRINK);

	vbox.pack_start(hbox, Gtk::PACK_EXPAND_WIDGET);

	vbox.pack_start(compress_progress_bar, Gtk::PACK_SHRINK);

	add(vbox);

	log_list.get_selection()->signal_changed().connect(sigc::mem_fun(this, &LogLauncher::on_log_list_selection_changed));
	on_log_list_selection_changed();
	analyzer_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_analyzer_clicked));
	player_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_player_clicked));
	rename_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_rename_clicked));
	delete_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_delete_clicked));
	export_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_export_clicked));
	import_button.signal_clicked().connect(sigc::mem_fun(this, &LogLauncher::on_import_clicked));

	show_all();

	compress_lock_fd = FileDescriptor::create_open(filename_to_pathname(".lock").c_str(), O_RDWR | O_CREAT, 0666);
	if (lockf(compress_lock_fd.fd(), F_TLOCK, 0) < 0) {
		if (errno == EAGAIN || errno == EACCES) {
			// File is already locked. Just do nothing.
			next_file_to_compress = files_to_compress.end();
		} else {
			throw SystemError("lockf", errno);
		}
	} else {
		// Delete old dotfiles in the logs directory.
		{
			Glib::Dir dir(get_logs_dir());
			std::vector<std::string> to_delete;
			std::copy_if(dir.begin(), dir.end(), std::back_inserter(to_delete), [](const std::string &fn) { return !fn.empty() && fn[0] == '.' && fn != ".lock"; });
			std::for_each(to_delete.begin(), to_delete.end(), [](const std::string &fn) { std::remove(filename_to_pathname(fn).c_str()); });
		}

		// Check which files need compressing.
		for (const std::string &i : files) {
			if (LogLoader::needs_compressing(filename_to_pathname(i))) {
				files_to_compress.push_back(i);
			}
		}

		if (!files_to_compress.empty()) {
			// Start compressing them.
			next_file_to_compress = files_to_compress.begin();
			rename_button.set_sensitive(false);
			delete_button.set_sensitive(false);
			compress_dispatcher.connect(sigc::mem_fun(this, &LogLauncher::start_compressing));
			start_compressing();
		} else {
			// Drop the lockfile so another process can grab it if desired.
			compress_lock_fd.close();
			next_file_to_compress = files_to_compress.end();
		}
	}
}

void LogLauncher::populate() {
	assert(compress_threads.empty());

	log_list.clear_items();
	files.clear();
	files_to_compress.clear();
	compress_progress_bar.set_text(Glib::ustring());
	compress_progress_bar.set_fraction(0);

	get_filenames(files);
	for (const std::string &i : files) {
		log_list.append(Glib::filename_display_basename(filename_to_pathname(i)));
	}
}

void LogLauncher::start_compressing() {
	// Join and erase any threads that are finished.
	{
		std::lock_guard<std::mutex> lock(compress_threads_done_mutex);
		for (std::thread::id i : compress_threads_done) {
			for (auto j = compress_threads.begin(), jend = compress_threads.end(); j != jend; ++j) {
				if (j->get_id() == i) {
					j->join();
					compress_threads.erase(j);
					break;
				}
			}
		}
		compress_threads_done.clear();
	}

	// Update progress bar to show # of files completely finished (excludes those left to compress *and* those currently compressing).
	std::size_t files_done = static_cast<std::size_t>(std::distance(static_cast<const std::vector<std::string> &>(files_to_compress).begin(), next_file_to_compress)) - compress_threads.size();
	compress_progress_bar.set_text(Glib::ustring::compose(u8"Compressed %1 / %2", files_done, files_to_compress.size()));
	compress_progress_bar.set_fraction(static_cast<double>(files_done) / static_cast<double>(files_to_compress.size()));

	// Fork more threads as long as we're supposed to.
	while (!exit_pending && next_file_to_compress != files_to_compress.end() && (compress_threads.empty() || compress_threads.size() < std::thread::hardware_concurrency())) {
		compress_threads.push_back(std::thread(std::bind(std::mem_fun(&LogLauncher::compress_thread_proc), this, *next_file_to_compress)));
		++next_file_to_compress;
	}

	// Check if we're done compressing everything.
	if (compress_threads.empty()) {
		if (exit_pending) {
			hide();
		} else if (next_file_to_compress == files_to_compress.end()) {
			// Done compressing all files.
			compress_progress_bar.set_text(Glib::ustring());
			compress_progress_bar.set_fraction(0);
			rename_button.set_sensitive();
			delete_button.set_sensitive();
			compress_lock_fd.close();
		}
	}
}

void LogLauncher::compress_thread_proc(const std::string &filename) {
	// Open the temporary file.
	const std::string &temp_filename = Glib::filename_from_utf8(Glib::ustring::compose(u8".loglauncher.tmp.%1", std::this_thread::get_id()));
	const std::string &temp_pathname = filename_to_pathname(temp_filename);
	FileDescriptor dst_fd = FileDescriptor::create_open(temp_pathname.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);

	{
		// Open the source file and memory map it.
		const MappedFile src_mapping(filename_to_pathname(filename));

		// BZip2 compression states:
		// “To guarantee that the compressed data will fit in its buffer, allocate an output buffer of size 1% larger than the uncompressed data, plus six hundred extra bytes.”
		// We do this by truncating the file and then mapping it.
		if (ftruncate(dst_fd.fd(), static_cast<off_t>(src_mapping.size() + (src_mapping.size() + 99) / 100 + 600)) < 0) {
			throw SystemError("ftruncate", errno);
		}

		// Memory map the destination file and do a buffer-to-buffer compress.
		{
			MappedFile dst_mapping(dst_fd, PROT_READ | PROT_WRITE);
			if (dst_mapping.size() > std::numeric_limits<unsigned int>::max()) {
				throw std::runtime_error("Log file too big.");
			}
			unsigned int dlen = static_cast<unsigned int>(dst_mapping.size());
			if (BZ2_bzBuffToBuffCompress(static_cast<char *>(dst_mapping.data()), &dlen, const_cast<char *>(static_cast<const char *>(src_mapping.data())), static_cast<unsigned int>(src_mapping.size()), 9, 0, 0) != BZ_OK) {
				throw std::runtime_error("BZip2 error compressing log file.");
			}
			dst_mapping.sync();

			// Now shorten the destination file to the amount of space actually used.
			if (ftruncate(dst_fd.fd(), dlen) < 0) {
				throw SystemError("ftruncate", errno);
			}
		}
	}

	// Sync the temporary file to disk and rename it over top of the original.
	if (fdatasync(dst_fd.fd()) < 0) {
		throw SystemError("fdatasync", errno);
	}
	if (std::rename(temp_pathname.c_str(), filename_to_pathname(filename).c_str()) < 0) {
		throw SystemError("rename", errno);
	}

	// Notify the manager that we're done.
	std::lock_guard<std::mutex> lock(compress_threads_done_mutex);
	compress_threads_done.push_back(std::this_thread::get_id());
	compress_dispatcher.emit();
}

bool LogLauncher::on_delete_event(GdkEventAny *) {
	if (compress_threads.empty()) {
		exit_pending = true;
		return false;
	} else {
		log_list.set_sensitive(false);
		analyzer_button.set_sensitive(false);
		player_button.set_sensitive(false);
		rename_button.set_sensitive(false);
		delete_button.set_sensitive(false);
		export_button.set_sensitive(false);
		import_button.set_sensitive(false);
		exit_pending = true;
		return true;
	}
}

void LogLauncher::on_log_list_selection_changed() {
	int num = log_list.get_selection()->count_selected_rows();
	analyzer_button.set_sensitive(num >= 1 && !exit_pending);
	player_button.set_sensitive(num >= 1 && !exit_pending);
	rename_button.set_sensitive(num == 1 && !exit_pending && next_file_to_compress == files_to_compress.end());
	delete_button.set_sensitive(num >= 1 && !exit_pending && next_file_to_compress == files_to_compress.end());
	export_button.set_sensitive(num == 1 && !exit_pending);
}

void LogLauncher::on_analyzer_clicked() {
	const std::vector<int> &selected = log_list.get_selected();
	for (int i : selected) {
		try {
			new LogAnalyzer(*this, filename_to_pathname(files[static_cast<std::size_t>(i)]));
		} catch (const std::runtime_error &exp) {
			Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.run();
		}
	}
}

void LogLauncher::on_player_clicked() {
	const std::vector<int> &selected = log_list.get_selected();
	for (int i : selected) {
		try {
			new LogPlayer(*this, filename_to_pathname(files[static_cast<std::size_t>(i)]));
		} catch (const std::runtime_error &exp) {
			Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			md.run();
		}
	}
}

void LogLauncher::on_rename_clicked() {
	Gtk::Dialog dlg(u8"Thunderbots Log Tools - Rename Log", *this, true);
	Gtk::HBox hbox;
	Gtk::Label label(u8"Enter new name for log:");
	hbox.pack_start(label, Gtk::PACK_SHRINK);
	Gtk::Entry new_name_entry;
	new_name_entry.set_activates_default();
	new_name_entry.set_width_chars(30);
	new_name_entry.set_text(log_list.get_text(static_cast<unsigned int>(log_list.get_selected()[0])));
	hbox.pack_start(new_name_entry, Gtk::PACK_EXPAND_WIDGET);
	hbox.show_all();
	dlg.get_vbox()->pack_start(hbox, Gtk::PACK_SHRINK);
	dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
	dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	dlg.set_default_response(Gtk::RESPONSE_OK);
	int resp = dlg.run();
	if (resp == Gtk::RESPONSE_OK) {
		const std::string &old_name = files[static_cast<std::size_t>(log_list.get_selected()[0])];
		const std::string &new_name = Glib::filename_from_utf8(new_name_entry.get_text());
		if (new_name != old_name) {
			const std::string &old_path = filename_to_pathname(old_name);
			const std::string &new_path = filename_to_pathname(new_name);
			try {
				if (std::rename(old_path.c_str(), new_path.c_str()) < 0) {
					throw SystemError("rename", errno);
				}
			} catch (const SystemError &exp) {
				Gtk::MessageDialog md(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
				md.run();
			}
			populate();
		}
	}
}

void LogLauncher::on_delete_clicked() {
	int num = log_list.get_selection()->count_selected_rows();
	Gtk::MessageDialog md(*this, Glib::ustring::compose(u8"Are you sure you wish to delete %1 %2 log%3?", num == 1 ? u8"this" : u8"these", num, num == 1 ? u8"" : u8"s"), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
	int resp = md.run();
	if (resp == Gtk::RESPONSE_YES) {
		const std::vector<int> &selected = log_list.get_selected();
		for (int i : selected) {
			const std::string &pathname = filename_to_pathname(files[static_cast<std::size_t>(i)]);
			try {
				if (std::remove(pathname.c_str()) < 0) {
					throw SystemError("remove", errno);
				}
			} catch (const SystemError &exp) {
				Gtk::MessageDialog md2(*this, exp.what(), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
				md2.run();
			}
		}
		populate();
	}
}

void LogLauncher::on_export_clicked() {
	const std::string &original_name = files[static_cast<std::size_t>(log_list.get_selected()[0])];
	Gtk::FileChooserDialog fcd(*this, u8"Export Log", Gtk::FILE_CHOOSER_ACTION_SAVE);
	fcd.set_do_overwrite_confirmation();
	fcd.set_current_name(Glib::filename_display_basename(original_name));
	fcd.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	fcd.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);
	if (fcd.run() == Gtk::RESPONSE_OK) {
		Gio::File::create_for_path(filename_to_pathname(original_name))->copy(fcd.get_file(), Gio::FILE_COPY_OVERWRITE);
	}
}

void LogLauncher::on_import_clicked() {
	Gtk::FileChooserDialog fcd(*this, u8"Import Log", Gtk::FILE_CHOOSER_ACTION_OPEN);
	fcd.set_select_multiple();
	fcd.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	fcd.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);
	if (fcd.run() == Gtk::RESPONSE_OK) {
		for (Glib::RefPtr<Gio::File> source : fcd.get_files()) {
			Glib::RefPtr<Gio::File> dest = Gio::File::create_for_path(filename_to_pathname(source->get_basename()));
			try {
				source->copy(dest, Gio::FILE_COPY_NONE);
			} catch (const Gio::Error &err) {
				if (err.code() == Gio::Error::EXISTS) {
					Gtk::MessageDialog md(*this, Glib::ustring::compose(u8"A log file named “%1” already exists. Replace it?", dest->query_info(G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME)->get_attribute_string(G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME)), false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
					if (md.run() == Gtk::RESPONSE_YES) {
						source->copy(dest, Gio::FILE_COPY_OVERWRITE);
					}
				} else {
					throw;
				}
			}
		}
		populate();
	}
}

