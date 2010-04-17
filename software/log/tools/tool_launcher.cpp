#include "log/reader/reader.h"
#include "log/tools/merger.h"
#include "log/tools/tool_launcher.h"
#include "log/tools/viewer.h"
#include <gtkmm.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>


namespace {
	class log_chooser : public Gtk::ListViewText, public noncopyable {
		public:
			log_chooser() : ListViewText(1, false, Gtk::SELECTION_MULTIPLE) {
				set_headers_visible(false);
				refresh();
			}

			std::vector<std::string> get_selected_logs() {
				const std::vector<int> &rows = get_selected();
				std::vector<std::string> v;
				for (std::vector<int>::const_iterator i = rows.begin(), iend = rows.end(); i != iend; ++i) {
					v.push_back(logs[*i]);
				}
				return v;
			}

			void refresh() {
				clear_items();
				logs = log_reader::all_logs();
				for (std::vector<std::string>::const_iterator i = logs.begin(), iend = logs.end(); i != iend; ++i) {
					append_text(Glib::filename_display_name(*i));
				}
			}

		private:
			std::vector<std::string> logs;
	};
}



class log_tool_launcher_impl : public Gtk::Window, public noncopyable {
	public:
		log_tool_launcher_impl() : box(false, 10), button_box(Gtk::BUTTONBOX_SPREAD), view_button("Play"), rename_button("Rename"), delete_button("Delete"), merge_button("Merge"), matlab_button("Create M file") {
			set_title("Log Tools");
			chooser.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::update_sensitivity));
			chooser_scroll.add(chooser);
			chooser_scroll.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
			box.pack_start(chooser_scroll, true, true);

			button_box.pack_start(view_button);
			button_box.pack_start(rename_button);
			button_box.pack_start(delete_button);
			button_box.pack_start(merge_button);
			button_box.pack_start(matlab_button);
			box.pack_start(button_box, false, true);

			add(box);
			show_all();

			update_sensitivity();
			view_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_view_clicked));
			rename_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_rename_clicked));
			delete_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_delete_clicked));
			merge_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_merge_clicked));
			matlab_button.signal_clicked().connect(sigc::mem_fun(*this, &log_tool_launcher_impl::on_matlab_clicked));
		}

	protected:
		bool on_delete_event(GdkEventAny *) {
			Gtk::Main::quit();
			return true;
		}

	private:
		Gtk::VBox box;

		Gtk::ScrolledWindow chooser_scroll;
		log_chooser chooser;

		Gtk::HButtonBox button_box;
		Gtk::Button view_button, rename_button, delete_button, merge_button, matlab_button;

		void update_sensitivity() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			if (sel.size() == 0) {
				view_button.set_sensitive(false);
				rename_button.set_sensitive(false);
				delete_button.set_sensitive(false);
				merge_button.set_sensitive(false);
				matlab_button.set_sensitive(false);
			} else if (sel.size() == 1) {
				view_button.set_sensitive(true);
				rename_button.set_sensitive(true);
				delete_button.set_sensitive(true);
				merge_button.set_sensitive(false);
				matlab_button.set_sensitive(true);
			} else {
				view_button.set_sensitive(false);
				rename_button.set_sensitive(false);
				delete_button.set_sensitive(true);
				merge_button.set_sensitive(true);
				matlab_button.set_sensitive(false);
			}
		}

		void on_view_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const std::string &name = sel[0];

			new log_viewer(name);
		}

		void on_rename_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const std::string &old_name = sel[0];

			Gtk::Dialog dlg("Rename Log", *this, true);
			Gtk::Label lbl("Enter the new name:");
			lbl.show();
			dlg.get_vbox()->pack_start(lbl);
			Gtk::Entry entry;
			entry.set_text(Glib::filename_to_utf8(old_name));
			entry.set_activates_default(true);
			entry.show();
			dlg.get_vbox()->pack_start(entry);
			dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
			dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dlg.set_default_response(Gtk::RESPONSE_OK);
			if (dlg.run() == Gtk::RESPONSE_OK) {
				const std::string &new_name = Glib::filename_from_utf8(entry.get_text());
				if (!new_name.empty()) {
					const std::string &dir = Glib::get_user_data_dir() + "/thunderbots/";
					if (rename((dir + old_name + ".log").c_str(), (dir + new_name + ".log").c_str()) < 0) {
						throw std::runtime_error("Cannot rename log file!");
					}
					if (rename((dir + old_name + ".idx").c_str(), (dir + new_name + ".idx").c_str()) < 0) {
						throw std::runtime_error("Cannot rename index file!");
					}
				}
				chooser.refresh();
			}
		}

		void on_matlab_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const std::string &name = sel[0];
			Gtk::FileChooserDialog dlg(*this, "Choose an output Matlab file", Gtk::FILE_CHOOSER_ACTION_SAVE);
			dlg.set_select_multiple(false);
			dlg.set_current_name(Glib::filename_to_utf8(name) + ".m");
			Gtk::FileFilter *filter = new Gtk::FileFilter();
			filter->set_name("Matlab/Octave Files");
			filter->add_pattern("*.m");
			dlg.add_filter(*filter);
			filter = new Gtk::FileFilter();
			filter->set_name("All Files");
			filter->add_pattern("*");
			dlg.add_filter(*filter);
			dlg.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			dlg.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
			if (dlg.run() == Gtk::RESPONSE_OK) {
				const std::string &new_name = Glib::filename_from_utf8(dlg.get_filename());

				////////////////////////////////////////////////////
				// We have a file name go ahead and create files  //
				////////////////////////////////////////////////////
				std::ofstream file(new_name.c_str());

				//////////////////////////
				// Create the log reader//
				//////////////////////////
				Glib::RefPtr<log_reader> reader = log_reader::create(name);

				//////////////////////////////////////////////
				//slightly hackey initilization of vars here//
				//////////////////////////////////////////////
				const int num_teams = 2;
				const int num_vals_robot = 3;
				const int num_vals_ball = 2;		

				const std::string teams[num_teams] = {"team_A", "team_B"};
				const std::string robotVals[num_vals_robot] = {"robotX", "robotY", "robotOrientation"};

				const std::string ballVals[num_vals_ball] = {"ballX", "ballY"};			

				double values_robot[num_vals_robot];

				const team::ptr teamz[2] = {reader->get_west_team(), reader->get_east_team()};

				/////////////////////////////////////////////////////
				//write data out as an bunch of octave 2D matricies//
				/////////////////////////////////////////////////////
				for (int g = 0; g < num_teams; g++) {
					const std::string &team = teams[g];
					for (int h = 0; h < num_vals_robot; h++) {
						reader->seek(0);//set the reader to the first frame
						file << team << robotVals[h] << " = [";
						for (std::size_t i = 0; i < reader->size(); i++) {
							////////////////////////////////////////////////////////////////////////////////////////
							//for this frame output the players "h th" property (eg pos().x pos().y orientation() //
							////////////////////////////////////////////////////////////////////////////////////////
							for (std::size_t j = 0; j < teamz[g]->size(); j++) {
								if (i > 0 && j == 0) file << ';';
								if (j > 0) file << ',';
								values_robot[0] = teamz[g]->get_robot(j)->position().x;						
								values_robot[1] = teamz[g]->get_robot(j)->position().y;
								values_robot[2] = teamz[g]->get_robot(j)->orientation();
								file << values_robot[h];
							}
							//////////////////////
							//advance the frame //
							//////////////////////
							reader->next_frame();

						}
						file << ']' << std::endl;
					} 
				}
				///////////////////////////////////
				// Done writing robots to file =)//
				///////////////////////////////////

				//////////////////////
				//write out the ball//
				//////////////////////
				for (int h = 0; h < num_vals_ball; h++) {
					reader->seek(0);//set the reader to the first frame
					file << ballVals[h] << " = [";
					for (std::size_t i = 0; i < reader->size(); i++) {
						if (i > 0) file << ',';
						if (h == 0) {
							file << reader->get_ball()->position().x;
						} else if (h == 1) {
							file << reader->get_ball()->position().y;
						} else {
							//throw exception?
						}
						//////////////////////
						//advance the frame //
						//////////////////////
						reader->next_frame();
					}
					file << ']' << std::endl;
				} 
				/////////////////////////////////
				// Done writing ball to file =)//
				/////////////////////////////////
			}
		}

		void on_delete_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			const Glib::ustring &msg = Glib::ustring::compose("Are you sure you want to delete %1 log%2?", sel.size(), sel.size() == 1 ? "" : "s");
			Gtk::MessageDialog dlg(*this, msg, false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
			dlg.set_title("Delete Logs");
			int rc = dlg.run();
			if (rc == Gtk::RESPONSE_YES) {
				for (std::vector<std::string>::const_iterator i = sel.begin(), iend = sel.end(); i != iend; ++i) {
					const std::string &base = Glib::get_user_data_dir() + "/thunderbots/" + *i;
					const std::string &logfile = base + ".log";
					const std::string &idxfile = base + ".idx";
					unlink(logfile.c_str());
					unlink(idxfile.c_str());
				}
				chooser.refresh();
			}
		}

		void on_merge_clicked() {
			const std::vector<std::string> &sel = chooser.get_selected_logs();
			log_merger *m = new log_merger(sel, *this);
			m->signal_merge_complete().connect(sigc::mem_fun(chooser, &log_chooser::refresh));
		}
};



log_tool_launcher::log_tool_launcher() : impl(new log_tool_launcher_impl) {
}

log_tool_launcher::~log_tool_launcher() {
	delete impl;
}

