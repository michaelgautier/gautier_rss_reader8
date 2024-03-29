/*
Copyright (C) 2020 Michael Gautier

This source code is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.

This source code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with this library. If not, see <http://www.gnu.org/licenses/>.

Author: Michael Gautier <michaelgautier.wordpress.com>
*/

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cuchar>
#include <cwchar>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include <webkit2/webkit2.h>

#include "rss_lib/rss_download/feed_download.hpp"
#include "rss_lib/rss/rss_feed_mod.hpp"
#include "rss_lib/rss/rss_reader.hpp"
#include "rss_lib/rss/rss_util.hpp"
#include "rss_lib/rss/rss_writer.hpp"

#include "rss_ui/application.hpp"
#include "rss_ui/app_win/app_win.hpp"
#include "rss_ui/app_win/article_frame.hpp"
#include "rss_ui/app_win/article_header.hpp"
#include "rss_ui/app_win/headlines_frame.hpp"

#include "rss_ui/rss_manage/rss_manage.hpp"

namespace ns_data_read = gautier_rss_data_read;
namespace ns_data_write = gautier_rss_data_write;

using single_article_by_feed_type = std::map<std::string, ns_data_read::rss_article>;

namespace {
	extern "C" {
		void headline_view_switch_page (GtkNotebook * rss_tabs, GtkWidget * page, guint page_num, gpointer user_data);

		void manage_feeds_click (GtkButton * button, gpointer user_data);
		void rss_operation_click (GtkButton * button, gpointer user_data);

		void select_headline_row (GtkTreeSelection * tree_selection, gpointer user_data);

		void window_size_allocate (GtkWidget * widget, GdkRectangle * allocation, gpointer user_data);
		void window_destroy (GtkWidget * window, gpointer user_data);
	}

	/*
		User Interface Pointers
	*/
	GtkWidget*
	headlines_view = nullptr;

	GtkWidget*
	header_bar = nullptr;

	GtkWidget*
	article_date = nullptr;

	GtkWidget*
	article_summary = nullptr;

	GtkWidget*
	article_details = nullptr;

	GtkWidget*
	info_bar = nullptr;

	GtkWidget*
	view_article_button = nullptr;

	GtkWidget*
	manage_feeds_button = nullptr;

	GtkWindow*
	win = nullptr;

	GtkWidget*
	layout_pane = nullptr;

	std::thread
	thread_download_data;

	std::thread
	thread_synchronize_ui;

	//C++ style enumeration.
	enum class
	rss_operation_enum
	{
		view_article
	};

	/*
		UI Window Construction
	*/
	void
	get_screen_dimensions (GtkWindow* window);

	void
	set_window_attributes (GtkWidget* window, const std::string title, const int width, const int height);

	void
	layout_rss_view (GtkWidget* window_layout, GtkWidget* rss_tabs, GtkWidget* articles_layout);

	void
	initialize_ui_threads();

	void
	synchronize_ui();

	/*
		Start-up (1)	Load the first batch of lines
		This is what produces a responsive UI on start-up when the
		size of the database exceeds what can be initially shown.
	*/
	void
	populate_rss_tabs();

	gboolean
	async_initialize_tabs (gpointer data);

	gboolean
	async_initialize_rss_management (gpointer data);

	gboolean
	flush_tabs (gpointer data);
	/*
		Start-up (2)	Loads batches of lines until all lines are loaded
		in each tab. This is calibrated to allow the end-user to operate the
		UI while the program continues to saturate each tab with all available
		information.
	*/
	gboolean
	async_load_tabs (gpointer data);

	/*
		Start-up (3)	Detects downloaded data (see RSS Download below)
		in each tab. This is calibrated to allow the end-user to operate the
		UI while the program continues to saturate each tab with all available
		information.
	*/
	gboolean
	async_load_tabs_with_downloaded_data (gpointer data);

	/*
		RSS Download

		Retrieves new RSS information and updates the database.
		Modifies the RSS Data Index to indicate the range of data uploaded.
	*/
	void
	download_data();

	void
	initialize_data_threads();

	/*
		RSS Data Index
	*/
	ns_data_read::feed_by_name_type
	feed_index;

	ns_data_read::feed_by_name_type
	downloaded_feeds;

	ns_data_write::headlines_by_feed_type
	feeds_articles;

	ns_data_write::headlines_by_feed_type
	downloaded_articles;

	single_article_by_feed_type
	feed_article_selection;

	/*
		Selected RSS Feed in a Tab
	*/
	ns_data_read::rss_article
	visible_feed_article;

	/*RSS Configuration Updates*/

	void
	process_rss_feed_configuration (const ns_data_read::rss_feed_mod modification);

	void
	synchronize_feeds_to_configuration (std::map<std::string, ns_data_read::rss_feed_mod> feed_changes);

	/*
		RSS Tabs - Switch Handlers.
	*/

	void
	show_article (const ns_data_read::rss_article article);

	void
	make_user_note (std::string note);

	rss_operation_enum
	rss_op_view_article = rss_operation_enum::view_article;

	const int64_t
	headline_max = 32;

	gulong
	headline_view_switch_page_signal_id = -1UL;

	gint
	next_notebook_tab_index = -1;

	int
	data_download_thread_wait_in_seconds = 6;

	int
	monitor_width = 0;

	int
	monitor_height = 0;

	int
	monitor_x = 0;

	int
	monitor_y = 0;

	int
	window_width = 0;

	int
	window_height = 0;

	bool
	feed_expire_time_enabled = false;

	/*Concurrency Control*/
	bool
	shutting_down = false;

	bool
	download_running = false;

	bool
	download_available = false;

	bool
	download_in_progress = false;

	bool
	rss_management_running = false;

	/*
		Window and Screen
	*/
	void
	get_screen_dimensions (GtkWindow* window)
	{
		GdkScreen* screen = gtk_window_get_screen (window);
		GdkWindow* screen_window = gdk_screen_get_root_window (screen);
		GdkDisplay* display = gdk_screen_get_display (screen);
		GdkMonitor* monitor = gdk_display_get_monitor_at_window (display, screen_window);

		GdkRectangle geometry;

		gdk_monitor_get_geometry (monitor, &geometry);

		const int scale_factor = gdk_window_get_scale_factor (screen_window);

		monitor_width = geometry.width * scale_factor;
		monitor_height = geometry.height * scale_factor;

		monitor_x = geometry.x;
		monitor_y = geometry.y;

		return;
	}

	void
	set_window_attributes (GtkWidget* window, const std::string title, const int width, const int height)
	{
		gtk_window_set_title (GTK_WINDOW (window), title.data());
		gtk_window_set_default_size (GTK_WINDOW (window), width, height);
		gtk_window_set_resizable (GTK_WINDOW (window), true);

		return;
	}

	void
	layout_rss_view (GtkWidget* window_layout, GtkWidget* rss_tabs, GtkWidget* articles_layout)
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		bool can_resize = true;
		bool can_shrink = true;

		/*
			Headlines Section
		*/
		gtk_paned_pack1 (GTK_PANED (window_layout), rss_tabs, can_resize, can_shrink);

		/*
			Article Section
		*/
		gtk_paned_pack2 (GTK_PANED (window_layout), articles_layout, can_resize, can_shrink);

		int
		default_width = ns_rss_tabs::get_default_headlines_view_content_width();

		gtk_paned_set_position (GTK_PANED (window_layout), default_width);

		return;
	}

	/*
		RSS Headlines Tab

		Tab page switch signal. Show news headlines for the chosen tab.
	*/
	void
	headline_view_switch_page (GtkNotebook* rss_tabs,
	                           GtkWidget*   content,
	                           guint        page_num,
	                           gpointer     user_data)
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		if (page_num > feed_index.size()) {
			std::cout << "Feed " << page_num << " with no in-memory source.\n";
		}

		if (user_data) {
			std::cout << __func__ << " called with user_data\n";
		}

		/*
			Get tab name.
		*/
		const std::string feed_name = gtk_notebook_get_tab_label_text (rss_tabs, content);

		make_user_note (feed_name + " RSS feed");

		/*
			Clear content.
		*/
		if (article_details) {
			GtkTextBuffer* text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (article_summary));

			std::string article_text = "";
			const size_t article_text_size = article_text.size();

			gtk_text_buffer_set_text (text_buffer, article_text.data(), (gint) (article_text_size));

			webkit_web_view_load_plain_text (WEBKIT_WEB_VIEW (article_details), article_text.data());

			const std::string url = "no feed data";

			gtk_widget_set_tooltip_text (view_article_button, url.data());
		}

		ns_rss_tabs::clear_row_selection (GTK_WIDGET (rss_tabs), feed_name);

		int64_t headline_count = 0;
		std::string date_status;

		if (feed_name.empty() == false) {
			const std::string db_file_name = gautier_rss_ui_app::get_db_file_name();

			ns_data_read::rss_feed feed;

			ns_data_read::get_feed (db_file_name, feed_name, feed);

			headline_count = feed.article_count;
			date_status = feed.last_retrieved;

			gtk_header_bar_set_title (GTK_HEADER_BAR (header_bar), feed_name.data());

			visible_feed_article = feed_article_selection[feed_name];

			/*
				Immediately designate feed name for row_select signal.
					Value of current_page is modified *after* signal handler returns.
					The *changed* signal handler has no visibiliity into the *proposed* value of *current_page*.
					Store feed name to provide a means to *peeK* into the proposed value of current_page.
			*/
			visible_feed_article.feed_name = feed_name;

			const std::string headline_text = visible_feed_article.headline;

			ns_rss_tabs::select_headline_row (GTK_WIDGET (rss_tabs), feed_name, headline_text);

			if (headline_count > 0) {
				make_user_note (feed_name + " " + std::to_string (headline_count) + " - articles since " + date_status);
			} else {
				make_user_note (feed_name + " RSS feed " + feed.feed_url);
			}
		}

		gtk_label_set_text (GTK_LABEL (article_date), date_status.data());

		if (headline_count > 0) {
			gautier_rss_win_main_headlines_frame::set_headlines_title (GTK_WIDGET (rss_tabs), (gint)page_num,
			        feed_name + " " + std::to_string (headline_count) + " records");
		} else {
			gautier_rss_win_main_headlines_frame::set_headlines_title (GTK_WIDGET (rss_tabs), (gint)page_num, feed_name);
		}

		return;
	}

	/*
		RSS Headlines List Box

		Choose a new headline.
	*/
	void
	select_headline_row (GtkTreeSelection* tree_selection, gpointer user_data)
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		if (tree_selection) {
			if (user_data) {
				std::cout << __func__ << " called with user_data\n";
			}

			const std::string db_file_name = gautier_rss_ui_app::get_db_file_name();
			const std::string feed_name = visible_feed_article.feed_name;

			if (feed_name.empty() == false) {
				std::string headline_text;

				ns_rss_tabs::get_selected_headline_text (tree_selection, headline_text);

				make_user_note (feed_name + " getting article ");

				if (headline_text.empty() == false) {
					visible_feed_article.headline = headline_text;

					ns_data_read::get_feed_article_summary (db_file_name, feed_name, headline_text, visible_feed_article);
				}

				ns_data_read::rss_feed feed;

				ns_data_read::get_feed (db_file_name, feed_name, feed);

				const std::string date_status = feed.last_retrieved;

				make_user_note (feed_name + " feed last updated " + date_status);
			}

			show_article (visible_feed_article);
		}

		return;
	}

	void
	show_article (const ns_data_read::rss_article article)
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		if (article.feed_name.empty() == false && article.url.empty() == false) {
			/*
				Update feed headline/article data.
			*/
			feed_article_selection[article.feed_name] = article;

			/*
				Article date.
			*/
			const std::string date_status = "Published -- " + article.article_date;
			gtk_label_set_text (GTK_LABEL (article_date), date_status.data());

			/*
				Article summary.
			*/
			{
				GtkTextBuffer* text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (article_summary));

				const std::string article_text = article.article_summary;
				const size_t article_text_size = article_text.size();

				gtk_text_buffer_set_text (text_buffer, article_text.data(), (gint) (article_text_size));
			}

			/*
				Article text.
			*/
			{
				if (article_details) {
					const std::string article_text = article.article_text;

					if (article_text.empty() == false) {
						webkit_web_view_load_html (WEBKIT_WEB_VIEW (article_details), article_text.data(), nullptr);
					} else {
						webkit_web_view_load_plain_text (WEBKIT_WEB_VIEW (article_details), article_text.data());
					}
				}
			}

			if (article.url.empty() == false) {
				gtk_widget_set_tooltip_text (view_article_button, article.url.data());
			}
		}

		return;
	}

	void
	rss_operation_click (GtkButton* button,
	                     gpointer   user_data)
	{
		if (button) {
			rss_operation_enum operation = * (rss_operation_enum*)user_data;

			if (operation == rss_operation_enum::view_article && visible_feed_article.url.empty() == false) {
				make_user_note ("Viewing " + visible_feed_article.feed_name + " in web browser");

				gtk_show_uri_on_window (win, visible_feed_article.url.data(), GDK_CURRENT_TIME, nullptr);
			}
		}

		return;
	}

	void
	manage_feeds_click (GtkButton* button,
	                    gpointer   user_data)
	{
		if (button) {
			if (user_data) {
				std::cout << __func__ << " called with user_data\n";
			}

			rss_management_running = true;

			gautier_rss_win_rss_manage::set_feed_model (feed_index);
			gautier_rss_win_rss_manage::set_modification_callback (synchronize_feeds_to_configuration);

			gautier_rss_win_rss_manage::show_dialog (win, window_width, window_height);
		}

		return;
	}

	void
	synchronize_feeds_to_configuration (std::map<std::string, ns_data_read::rss_feed_mod> feed_changes)
	{
		g_signal_handler_disconnect (headlines_view, headline_view_switch_page_signal_id);

		for (auto [feed_name, modification] : feed_changes) {
			const ns_data_read::rss_feed_mod_status status = modification.status;

			if (status != ns_data_read::rss_feed_mod_status::none) {
				process_rss_feed_configuration (modification);
			}
		}

		headline_view_switch_page_signal_id = g_signal_connect (headlines_view, "switch-page",
		                                      G_CALLBACK (headline_view_switch_page), nullptr);

		rss_management_running = false;

		return;
	}

	void
	process_rss_feed_configuration (const ns_data_read::rss_feed_mod modification)
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		const std::string db_file_name = gautier_rss_ui_app::get_db_file_name();

		ns_data_read::rss_feed_mod_status status = modification.status;

		const std::string feed_name = modification.feed_name;

		const bool is_insert = (status == ns_data_read::rss_feed_mod_status::insert);

		const size_t feed_index_match_count = feed_index.count (feed_name);

		if (feed_index_match_count < 1) {
			ns_data_read::rss_feed feed;

			get_feed (db_file_name, feed_name, feed);

			feed_index.insert_or_assign (feed_name, feed);

			ns_data_read::rss_feed* feed_clone = &feed_index[feed_name];
			feed_clone->last_index = -1;

			feeds_articles.insert_or_assign (feed_name, ns_data_read::headlines_list_type());
		}

		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		if (is_insert == false) {
			GtkWidget* tab = nullptr;

			gint tab_n = -1;

			const gint page_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

			if (page_count > 0) {
				for (gint tab_i = 0; tab_i < page_count; tab_i++) {
					tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

					const std::string tab_label = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (headlines_view), tab);

					if (feed_name == tab_label) {
						tab_n = tab_i;

						break;
					}
				}
			}

			if (tab_n > -1 && tab) {
				if (status == ns_data_read::rss_feed_mod_status::remove) {
					gtk_widget_hide (tab);

					make_user_note (feed_name + " DELETED.");

					gtk_notebook_remove_page (GTK_NOTEBOOK (headlines_view), tab_n);

					feed_index.erase (feed_name);
					feeds_articles.erase (feed_name);
				} else if (status ==  ns_data_read::rss_feed_mod_status::change) {
					const int64_t row_id = modification.row_id;

					ns_data_read::rss_feed updated_feed;

					ns_data_read::get_feed_by_row_id (db_file_name, row_id, updated_feed);

					const std::string updated_feed_name = updated_feed.feed_name;

					if (updated_feed_name != feed_name) {
						ns_data_read::rss_feed* feed_in_use = &feed_index[feed_name];

						gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (headlines_view), tab, updated_feed_name.data());
						/*Only changing name -- other changes will be cached to another queue and processed as appropriate.*/
						make_user_note (feed_name + " updated to " + updated_feed_name + ".");

						feed_index.insert_or_assign (updated_feed_name, updated_feed);

						ns_data_read::rss_feed* feed_clone = &feed_index[updated_feed_name];
						feed_clone->last_index = feed_in_use->last_index;

						feeds_articles.insert_or_assign (updated_feed_name, feeds_articles[feed_name]);

						feed_index.erase (feed_name);
						feeds_articles.erase (feed_name);
					}
				}

			}
		} else if (is_insert) {
			/*
				INSERT New Tab - Download feed entries
			*/
			gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

			ns_rss_tabs::add_headline_page (headlines_view, feed_name, tab_count + 1, select_headline_row);

			tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

			for (gint tab_i = 0; tab_i < tab_count; tab_i++) {
				GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

				if (tab == nullptr) {
					continue;
				}

				std::string tab_label = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (headlines_view), tab);

				ns_data_read::rss_feed* feed_in_use = &feed_index[feed_name];

				if (feed_in_use && feed_name == tab_label) {
					std::string feed_url = feed_in_use->feed_url;
					std::string retrieve_limit_hrs = feed_in_use->retrieve_limit_hrs;
					std::string retention_days = feed_in_use->retention_days;

					ns_data_read::headlines_list_type headlines;

					long response_code = ns_data_write::update_rss_db_from_network (db_file_name,
					                     feed_name,
					                     feed_url,
					                     retrieve_limit_hrs,
					                     retention_days,
					                     headlines);

					const bool network_response_good = ns_data_read::is_network_response_ok (response_code);

					/*
						Failure here is acceptable.
						The main download process will pick up any updates in a later thread.
						This is merely a convenience to show results early if possible.
						That is why this code is less elaborate compared to the main download process.
					*/
					if (network_response_good) {
						const int64_t in_use_count = ns_data_read::get_feed_headline_count (db_file_name, feed_name);

						const int64_t index_start = 0;
						const int64_t index_end = in_use_count - 1;

						ns_data_read::headline_range_type
						range = std::make_pair (index_start, index_end);

						if (index_start < index_end) {
							ns_rss_tabs::show_headlines (headlines_view, feed_name, range, headlines, true);

							gtk_widget_show_all (tab);
						}
					}

					gtk_widget_set_sensitive (tab, true);

					break;
				}
			}
		}

		return;
	}

	void
	window_size_allocate (GtkWidget* widget, GdkRectangle* allocation, gpointer user_data)
	{
		if (widget && allocation) {
			if (user_data) {
				std::cout << __func__ << " called with user_data\n";
			}

			window_width = 0;
			window_height = 0;

			/*
				GtkAllocation is a typedef for GdkRectangle
				The rectangle struct is defined as

					int x,y,width,height;
			*/
			if (allocation) {
				window_width = allocation->width;
				window_height = allocation->height;
			}
		}

		return;
	}

	void
	window_destroy (GtkWidget* window, gpointer user_data)
	{
		if (window) {
			if (user_data) {
				std::cout << __func__ << " called with user_data\n";
			}

			shutting_down = true;
			download_running = false;

			const bool data_thread_instance_exists = thread_download_data.joinable();

			if (data_thread_instance_exists) {
				thread_download_data.join();
			}

			const bool ui_thread_instance_exists = thread_synchronize_ui.joinable();

			if (ui_thread_instance_exists) {
				thread_synchronize_ui.join();
			}

			/*
				These use GObject explicit ref counting.
				Ref counting enables explicit clean-up sequence.
			*/
			if (article_details) {
				gtk_widget_destroy (article_details);
			}

			gtk_widget_destroy (article_date);
			gtk_widget_destroy (article_summary);
			gtk_widget_destroy (header_bar);
			gtk_widget_destroy (info_bar);
			gtk_widget_destroy (manage_feeds_button);
			gtk_widget_destroy (view_article_button);
			gtk_widget_destroy (headlines_view);
			gtk_widget_destroy (layout_pane);
		}

		return;
	}

	void
	populate_rss_tabs()
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		/*
			Make a tab for each feed name.
		*/
		make_user_note ("Setting up tabs");

		std::vector<ns_data_read::rss_feed> feed_names;

		const std::string db_file_name = gautier_rss_ui_app::get_db_file_name();

		ns_data_read::get_feeds (db_file_name, feed_names);

		ns_data_read::headlines_list_type headline_snapshot;

		for (ns_data_read::rss_feed feed : feed_names) {
			const std::string feed_name = feed.feed_name;

			feed_index.insert_or_assign (feed_name, feed);

			ns_data_read::rss_feed* feed_clone = &feed_index[feed_name];
			feed_clone->last_index = -1;

			ns_data_read::get_feed_headlines (db_file_name, feed_name, headline_snapshot, true);

			feeds_articles.insert_or_assign (feed_name, headline_snapshot);

			ns_rss_tabs::add_headline_page (headlines_view, feed_name, -1, select_headline_row);

			headline_snapshot.clear();
		}

		const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

		if (tab_count > 0) {
			const gint tab_i = 0;

			GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

			if (tab) {
				const std::string feed_name = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (headlines_view), tab);

				gautier_rss_win_main_headlines_frame::set_headlines_title (headlines_view, tab_i, feed_name);
			}
		}

		return;
	}

	/*
		UI threads
	*/
	void
	initialize_ui_threads()
	{
		thread_synchronize_ui = std::thread (synchronize_ui);

		return;
	}

	void
	synchronize_ui()
	{
		while (shutting_down == false) {
			const int async_tab_init_wait_in_milliseconds = 108;

			std::this_thread::sleep_for (std::chrono::milliseconds (async_tab_init_wait_in_milliseconds));

			const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

			const bool tabs_initialized = (next_notebook_tab_index >= tab_count);

			if (tabs_initialized) {
				break;
			} else {
				g_main_context_invoke (nullptr, async_initialize_tabs, nullptr);
			}

			next_notebook_tab_index++;
		}

		g_main_context_invoke (nullptr, async_initialize_rss_management, nullptr);

		initialize_data_threads();

		g_main_context_invoke (nullptr, flush_tabs, nullptr);

		size_t cycles = 0;
		bool download_update_active = false;

		next_notebook_tab_index = -1;

		const int default_async_wait_in_milliseconds = 208;
		int async_tab_load_wait_in_milliseconds = default_async_wait_in_milliseconds;

		while (shutting_down == false) {
			std::this_thread::sleep_for (std::chrono::milliseconds (async_tab_load_wait_in_milliseconds));

			next_notebook_tab_index++;
			cycles++;

			const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

			if (next_notebook_tab_index > tab_count) {
				next_notebook_tab_index = 0;
			}

			if (cycles >= 4 && cycles >= feed_index.size()) {
				cycles = 0;

				g_main_context_invoke (nullptr, flush_tabs, nullptr);

				async_tab_load_wait_in_milliseconds = (default_async_wait_in_milliseconds / 2);
			}

			int64_t all_articles_count = 0;

			if (feeds_articles.size() > 0) {
				for (auto [feed_name, headlines] : feeds_articles) {
					all_articles_count = all_articles_count + headlines.size();
				}
			}

			if (download_available) {
				if (download_update_active == false) {
					download_update_active = true;
					next_notebook_tab_index = 0;
				}

				g_main_context_invoke (nullptr, async_load_tabs_with_downloaded_data, nullptr);
			} else if (all_articles_count > 0) {
				if (download_update_active) {
					download_update_active = false;
					next_notebook_tab_index = 0;
				}

				g_main_context_invoke (nullptr, async_load_tabs, nullptr);
			} else if (all_articles_count == 0) {
				std::cout << " No feeds left to process\n";

				async_tab_load_wait_in_milliseconds = default_async_wait_in_milliseconds;

				break;
			}
		}

		while (shutting_down == false) {
			std::this_thread::sleep_for (std::chrono::milliseconds (async_tab_load_wait_in_milliseconds));

			next_notebook_tab_index++;
			cycles++;

			const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

			if (next_notebook_tab_index > tab_count) {
				next_notebook_tab_index = 0;
			}

			if (cycles >= 4 && cycles >= downloaded_feeds.size()) {
				cycles = 0;

				g_main_context_invoke (nullptr, flush_tabs, nullptr);

				async_tab_load_wait_in_milliseconds = (default_async_wait_in_milliseconds / 2);
			}

			int64_t all_articles_count = 0;

			if (feeds_articles.size() > 0) {
				for (auto [feed_name, headlines] : downloaded_articles) {
					all_articles_count = all_articles_count + headlines.size();
				}
			}

			if (download_available) {
				if (download_update_active == false) {
					download_update_active = true;
					next_notebook_tab_index = 0;
				}

				g_main_context_invoke (nullptr, async_load_tabs_with_downloaded_data, nullptr);
			} else if (all_articles_count > 0) {
				if (download_update_active) {
					download_update_active = false;
					next_notebook_tab_index = 0;

					g_main_context_invoke (nullptr, flush_tabs, nullptr);
				}

				async_tab_load_wait_in_milliseconds = default_async_wait_in_milliseconds;
			} else if (all_articles_count == 0) {
				async_tab_load_wait_in_milliseconds = (default_async_wait_in_milliseconds / 2);
			}
		}

		return;
	}

	gboolean
	async_initialize_tabs (gpointer data)
	{
		if (data) {
			std::cout << __func__ << " called with user_data\n";
		}

		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

		bool feed_exists = false;

		std::string feed_name;

		if (tab_count > 0) {
			const gint tab_i = next_notebook_tab_index;

			if (tab_i > -1) {
				GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

				if (tab) {
					feed_name = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (headlines_view), tab);

					make_user_note ("Setting up " + feed_name);

					feed_exists = ns_data_read::contains_feed (feed_index, feed_name);
				}
			}
		}

		if (feed_exists && feed_name.empty() == false) {
			const ns_data_read::headline_range_type range = acquire_headline_range (feed_name, feed_index, headline_max);

			if (ns_data_read::headline_range_valid (range)) {
				ns_rss_tabs::show_headlines (headlines_view, feed_name, range, feeds_articles[feed_name], false);
			}
		}

		return false;
	}

	gboolean
	async_initialize_rss_management (gpointer data)
	{
		if (data) {
			std::cout << __func__ << " called with user_data\n";
		}

		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		const std::string feed_name = ns_rss_tabs::get_selected_tab_name (headlines_view);

		const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

		for (gint tab_i = 0; tab_i <= tab_count; tab_i++) {
			GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

			if (tab) {
				gboolean sensitive_value = gtk_widget_get_sensitive (tab);

				if (sensitive_value == false) {
					gtk_widget_set_sensitive (tab, true);
				}
			}

			const bool tab_visible = ns_rss_tabs::is_tab_selected (headlines_view, feed_name);

			if (tab_visible) {
				visible_feed_article.feed_name = feed_name;
			}
		}

		/*
			Tab page switch signal. Show news headlines for the chosen tab.
		*/
		headline_view_switch_page_signal_id = g_signal_connect (headlines_view, "switch-page",
		                                      G_CALLBACK (headline_view_switch_page), nullptr);

		gtk_widget_set_sensitive (manage_feeds_button, true);

		data_download_thread_wait_in_seconds = 4;
		download_running = true;

		return false;
	}

	gboolean
	async_load_tabs (gpointer data)
	{
		if (data) {
			std::cout << __func__ << " called with user_data\n";
		}

		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

		bool feed_exists = false;

		std::string feed_name;

		const bool download_active = (download_available || download_in_progress);

		if (download_active == false && tab_count > 0) {
			const gint tab_i = next_notebook_tab_index;

			if (next_notebook_tab_index > tab_count) {
				next_notebook_tab_index = 0;
			}

			if (tab_i > -1) {
				GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

				if (tab) {
					feed_name = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (headlines_view), tab);

					feed_exists = ns_data_read::contains_feed (feed_index, feed_name);
				}
			}
		}

		const bool tab_visible = ns_rss_tabs::is_tab_selected (headlines_view, feed_name);

		if (download_active == false && feed_exists && feed_name.empty() == false) {
			ns_data_read::headline_range_type range = acquire_headline_range (feed_name, feed_index, headline_max);

			if (ns_data_read::headline_range_valid (range)) {
				ns_rss_tabs::show_headlines (headlines_view, feed_name, range, feeds_articles[feed_name], false);

				if (tab_visible) {
					make_user_note (feed_name +  " added record " + std::to_string (range.second));
				}
			} else {
				/*
					Reclaim the memory consumed by the snapshot.

					No further need for it in the current program design (3/2/2020).
				*/
				gautier_rss_data_read::headlines_list_type* headlines = &feeds_articles[feed_name];

				headlines->clear();
				headlines->shrink_to_fit();

				feeds_articles.erase (feed_name);
			}
		}

		return false;
	}

	gboolean
	async_load_tabs_with_downloaded_data (gpointer data)
	{
		std::cout << __FILE__ << " \t\t\t\t\t" << __func__ << " feed \t\t ENTER  \n";

		if (data) {
			std::cout << __func__ << " called with user_data\n";
		}

		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

		bool feed_exists = false;

		std::string feed_name;

		if (download_available && tab_count > 0) {
			const gint tab_i = next_notebook_tab_index;

			if (next_notebook_tab_index > tab_count) {
				next_notebook_tab_index = 0;
			}

			if (tab_i > -1) {
				GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

				if (tab) {
					feed_name = gtk_notebook_get_tab_label_text (GTK_NOTEBOOK (headlines_view), tab);

					feed_exists = ns_data_read::contains_feed (downloaded_feeds, feed_name);
				}
			}
		}

		const bool tab_visible = ns_rss_tabs::is_tab_selected (headlines_view, feed_name);

		if (feed_exists && feed_name.empty() == false) {
			ns_data_read::headline_range_type range = acquire_headline_range (feed_name, downloaded_feeds, headline_max);

			if (ns_data_read::headline_range_valid (range)) {
				ns_rss_tabs::show_headlines (headlines_view, feed_name, range, downloaded_articles[feed_name], true);

				if (tab_visible) {
					make_user_note (feed_name + " downloaded record " + std::to_string (range.second));
				}
			} else {
				gautier_rss_data_read::headlines_list_type* headlines = &downloaded_articles[feed_name];

				headlines->clear();
				headlines->shrink_to_fit();

				downloaded_feeds.erase (feed_name);
				downloaded_articles.erase (feed_name);

				const size_t feed_count = downloaded_feeds.size();

				download_available = (feed_count > 0);
			}
		} else {
			if (tab_visible) {
				if (feed_article_selection.count (feed_name) < 1) {
					ns_rss_tabs::select_headline_row (GTK_WIDGET (headlines_view), feed_name, "");
				}
			}
		}

		return false;
	}

	gboolean
	flush_tabs (gpointer data)
	{
		if (data) {
			std::cout << __func__ << " called with user_data\n";
		}

		const gint tab_count = gtk_notebook_get_n_pages (GTK_NOTEBOOK (headlines_view));

		for (gint tab_i = 0; tab_i < tab_count; tab_i++) {
			GtkWidget* tab = gtk_notebook_get_nth_page (GTK_NOTEBOOK (headlines_view), tab_i);

			if (tab) {
				gtk_widget_show_all (tab);
			}
		}

		return false;
	}

	void
	make_user_note (std::string note)
	{
		gtk_label_set_text (GTK_LABEL (info_bar), note.data());

		return;
	}

	/*
		Database threads - NO UI
	*/
	void
	initialize_data_threads()
	{
		thread_download_data = std::thread (download_data);

		return;
	}

	/*
		RSS DATA INTEGRATION - The **PRIMARY** RSS function.
		Designed to run in a separate thread. All it does is update a database.
		The user interface is expected to have its own logic to detect database
		modifications that occur when function executes.

		This function does not interact with the user interface.
	*/
	void
	download_data()
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		/*
			KEEP ALL the std::cout << calls. They are an intentional part of
			this function. That way, when the program is initiated from a
			command-line, the primary output to stdout is network/database
			processes that occur in this function.
		*/

		const std::string db_file_name = gautier_rss_ui_app::get_db_file_name();

		while (shutting_down == false && download_running == false) {
			std::cout << __func__ << ":\tentering sleep\n";
			std::this_thread::sleep_for (std::chrono::seconds (data_download_thread_wait_in_seconds));
		}

		std::cout << __func__ << ":\tawake\n";

		int successful_download_attempts = 0;
		int failed_download_attempts = 0;
		const int max_failed_download_attempts = 9;
		bool failed_download_notify_was_output = false;
		int change_count = 0;
		std::string last_download_datetime = gautier_rss_util::get_current_date_time_utc();
		std::string last_failed_download_datetime;

		/*
			Execution is signaled by download_running == true
			When that condition changes, exit is defined in 3 key areas.
			If a download is in progress, that download will conclude but no other will initiate
			when the end-user signals the end of the application.
		*/
		int allow_process_output = true;

		data_download_thread_wait_in_seconds = 2;

		while (shutting_down == false && download_running) {
			std::this_thread::sleep_for (std::chrono::seconds (data_download_thread_wait_in_seconds));

			std::vector <ns_data_read::rss_feed> feeds;

			ns_data_read::get_feeds (db_file_name, feeds);

			if (feeds.size() < 1 ||
			        rss_management_running ||
			        download_available) {
				continue;
			}

			/*
				End this thread if the user closes the program.
			*/
			if (shutting_down || download_running == false) {
				std::cout << __func__ << ", LINE: " << __LINE__ << ";\tEXIT\n";
				break;
			}

			if (allow_process_output) {
				std::cout << __func__ << ":\tPreparing to download\n";
			}

			/*
				If downloads have already occurred within a hour window of time,
				stop further download processing until 12 minutes has passed.
			*/
			if (successful_download_attempts > 0) {
				if (download_in_progress) {
					download_in_progress = false;
				}

				const int download_restart_wait_in_minutes = 5;
				const int wait_time_in_seconds = (download_restart_wait_in_minutes * 60);

				const std::string prep_download_datetime = gautier_rss_util::get_current_date_time_utc();

				const int_fast32_t time_difference_in_seconds = gautier_rss_util::get_time_difference_in_seconds (
				            last_download_datetime, prep_download_datetime);

				const bool allow_time_output = (time_difference_in_seconds <= 4 ||
				                                time_difference_in_seconds >= (wait_time_in_seconds + 2));

				if (allow_time_output) {
					std::cout << __func__ << ", LINE: " << __LINE__ << ";\t\tTime check\n";

					std::cout << __func__ << ", LINE: " << __LINE__ << ";\t\t\tSeconds since last successful download: " <<
					          time_difference_in_seconds << "\n";
				}

				/*
					Any return to the loop entrance puts this just past 720s (12 minutes) once the time reaches 12 minutess.

					This is the *Guarantee that the application will not access the network too frequently but will achieve
					also check for data often enough in the event there is new data based on the feed's last retrieved date.

					Website operators tend to dislike programs that check the website every few seconds. Setting this
					to an hour guarantees that the program will not unintentionally violate minimum website access intervals.
				*/
				const bool clock_still_running = (time_difference_in_seconds < (wait_time_in_seconds + 2));

				if (clock_still_running) {
					if (allow_time_output) {
						std::cout << __func__ << ", LINE: " << __LINE__ << ";\t\t\tHOLD for " << wait_time_in_seconds << "s.\n";
					}

					continue;
				} else {
					allow_process_output = true;
				}
			}

			if (failed_download_attempts >= max_failed_download_attempts) {
				const int download_retry_wait_in_minutes = 5;
				const int wait_time_in_seconds = (download_retry_wait_in_minutes * 60);

				const std::string download_review_datetime = gautier_rss_util::get_current_date_time_utc();

				const int_fast32_t time_difference_in_seconds = gautier_rss_util::get_time_difference_in_seconds (
				            last_failed_download_datetime, download_review_datetime);

				if (failed_download_notify_was_output == false) {
					std::cout << __func__ << ", LINE: " << __LINE__ << ";\tSeveral failed downloads (unable to connect).\n";
					std::cout << __func__ << ", LINE: " << __LINE__ << ";\t\tWill try again in " << download_retry_wait_in_minutes
					          << " minutes.\n";
				}

				const bool clock_still_running = (time_difference_in_seconds <= (wait_time_in_seconds + 2));

				if (clock_still_running) {
					if (failed_download_notify_was_output == false) {
						failed_download_notify_was_output = true;

						std::cout << __func__ << ", LINE: " << __LINE__ << ";\t\t\t\tSKIP for " << wait_time_in_seconds << "s.\n";
					}

					continue;
				} else {
					allow_process_output = true;
					failed_download_attempts = 0;
				}
			}

			/*
				Feeds with new data.
			*/
			change_count = 0;
			successful_download_attempts = 0;

			/*
				Cycles through all the feeds to download, 1 at a time.
			*/
			for (ns_data_read::rss_feed feed : feeds) {
				const std::string feed_name = feed.feed_name;
				const std::string feed_url = feed.feed_url;
				const std::string retrieve_limit_hrs = feed.retrieve_limit_hrs;
				const std::string retention_days = feed.retention_days;

				if (feed_name.empty() || feed_url.empty()) {
					continue;
				}

				/*
					End this thread if the user closes the program.
				*/
				if (shutting_down || download_running == false) {
					break;
				}

				std::cout << "DOWNLOAD ATTEMPT FOR FEED: \t\t\t\t ********** " <<  feed_name << " ********** \n";

				/*
					Aborts the download attempt if a download has already occured within the allowed time frame.
				*/
				const bool is_feed_still_fresh = ns_data_read::is_feed_still_fresh (db_file_name, feed_name,
				                                 feed_expire_time_enabled);

				/*
					Handles the situation where the program is relaunched within the normal minimum website access time frame.
					In that case, use the most recent retrieve date stored in the feed's record.
				*/
				if (is_feed_still_fresh) {
					last_download_datetime = feed.last_retrieved;
					successful_download_attempts++;
				}

				if (is_feed_still_fresh == false && (shutting_down == false && download_running)) {
					ns_data_read::headlines_list_type articles;

					/*
						The download of a given feed will be retried a few times.
					*/
					const int max_download_attempts = 3;
					int download_attempts = 0;

					bool network_response_good = false;

					download_in_progress = true;

					while (network_response_good == false && download_attempts < max_download_attempts) {
						download_attempts++;

						const long response_code = ns_data_write::update_rss_db_from_network (db_file_name,
						                           feed_name,
						                           feed_url,
						                           retrieve_limit_hrs,
						                           retention_days,
						                           articles);

						network_response_good = ns_data_read::is_network_response_ok (response_code);

						if (network_response_good) {
							successful_download_attempts++;

							std::cout << "DOWNLOAD SUCCESS FOR FEED: \t\t\t\t ********** " <<  feed_name << " [GOOD] ***\n";

							/*Although there is control logic in the loop, go ahead in this case and exit early.*/
							break;
						} else {
							if (download_attempts == max_download_attempts) {
								failed_download_attempts++;

								if (failed_download_attempts >= max_failed_download_attempts) {
									last_failed_download_datetime = gautier_rss_util::get_current_date_time_utc();

									allow_process_output = false;
									failed_download_notify_was_output = false;
								}

								std::cout << "DOWNLOAD FAILURE FOR FEED: \t\t\t\t ********** " <<  feed_name << " [FAIL] ***\n";
							}

							/*Skip this iteration since no data is expected*/
							continue;
						}
					}

					/*
						Transfer new data to cache.

						A 'UI thread valid for updating the UI' will pick up these values.
					*/
					ns_data_read::rss_feed feed_new;

					ns_data_read::get_feed (db_file_name, feed_name, feed_new);

					const bool new_updates = ns_data_read::check_feed_changed (feed, feed_new);

					if (new_updates) {
						change_count++;

						downloaded_feeds.insert_or_assign (feed_name, feed_new);

						downloaded_articles.insert_or_assign (feed_name, articles);

						ns_data_read::rss_feed* updated_feed = &downloaded_feeds[feed_name];

						updated_feed->last_index = -1;
					}
				}
			}

			/*
				A signal to this thread that no more downloads should be attempted for a while.
			*/
			if (successful_download_attempts > 0) {
				last_download_datetime = gautier_rss_util::get_current_date_time_utc();

				std::cout << "DOWNLOAD COMPLETE (" << last_download_datetime << ") \t\t\t"
				          << downloaded_feeds.size() << " feeds:\n";

				for (auto [downloaded_feed_name, downloaded_feed] : downloaded_feeds) {
					std::cout << "\t\t\t\t\t"
					          << downloaded_feed_name
					          << "\t\t" << downloaded_feed.feed_url
					          << " \t\tLAST RETRIEVED: "
					          << downloaded_feed.last_retrieved << "\n";
				}

				download_available = (downloaded_feeds.size() > 0);

				allow_process_output = false;
			}

			download_in_progress = false;
		}

		return;
	}
}

/*
	Application GUI Entry Point. The program starts here.
*/
void
gautier_rss_win_main::create (
    GtkApplication* application, gpointer user_data)
{
	/*
		Operating in a valid instance of a GTK application.
	*/
	if (application && user_data) {
		std::cout << "UI Initialized: " << gautier_rss_util::get_current_date_time_utc() << "\n";
	}

	/*
		Window
	*/
	GtkWidget* window = gtk_application_window_new (application);

	g_signal_connect (window, "size-allocate", G_CALLBACK (window_size_allocate), nullptr);
	g_signal_connect (window, "destroy", G_CALLBACK (window_destroy), nullptr);

	win = GTK_WINDOW (window);
	get_screen_dimensions (win);
	set_window_attributes (window, gautier_rss_ui_app::get_application_name(), monitor_width, monitor_height);

	/*
		Header Bar
	*/
	header_bar = gtk_header_bar_new();
	g_object_ref_sink (header_bar);
	gautier_rss_ui_app::set_css_class (header_bar, "header_bar");
	{
		namespace ns = gautier_rss_win_main_article_header;
		ns::initialize_article_header_bar (header_bar);
	}

	/*
		Article Summary
	*/
	article_summary = gtk_text_view_new();
	g_object_ref_sink (article_summary);
	gautier_rss_ui_app::set_css_class (article_summary, "article_summary");
	{
		gtk_text_view_set_editable (GTK_TEXT_VIEW (article_summary), false);
		gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (article_summary), false);
		gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (article_summary), GTK_WRAP_WORD_CHAR);
		gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (article_summary), true);
	}

	/*
		Article Text
	*/
	{
		WebKitSettings* settings = webkit_settings_new();

		/*Do not want images, just HTML*/
		webkit_settings_set_auto_load_images (settings, false);

		/*The objective is to render HTML, but no JavaScript, local HTML databases, etc*/
		webkit_settings_set_enable_html5_database (settings, false);
		webkit_settings_set_enable_html5_local_storage (settings, false);
		webkit_settings_set_enable_java (settings, false);
		webkit_settings_set_enable_javascript (settings, false);
		webkit_settings_set_enable_offline_web_application_cache (settings, false);
		//webkit_settings_set_enable_plugins (settings, false); /*GTK+ 3 marks this as deprecated since version 2.32*/
		webkit_settings_set_enable_webaudio (settings, false);
		webkit_settings_set_enable_webgl (settings, false);
		webkit_settings_set_enable_page_cache (settings, false);
		webkit_settings_set_enable_media_stream (settings, false);

		WebKitWebContext* web_context = webkit_web_context_new_ephemeral();
		article_details = webkit_web_view_new_with_context (web_context);
		g_object_ref_sink (article_details);
		webkit_web_view_set_settings (WEBKIT_WEB_VIEW (article_details), settings);
	}

	/*
		Article Date
	*/
	article_date = gtk_label_new (nullptr);
	g_object_ref_sink (article_date);
	gautier_rss_ui_app::set_css_class (article_date, "article_date");

	gtk_widget_set_halign (article_date, GTK_ALIGN_END);

	/*
		Primary Functions
	*/
	GtkWidget* primary_function_buttons = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gautier_rss_ui_app::set_css_class (primary_function_buttons, "rss_main_button_container");
	gtk_button_box_set_layout (GTK_BUTTON_BOX (primary_function_buttons), GTK_BUTTONBOX_START);
	{
		/*
			View Article
		*/
		view_article_button = gtk_button_new_with_label ("View Article");
		gautier_rss_ui_app::set_css_class (view_article_button, "button");

		g_signal_connect (view_article_button, "clicked", G_CALLBACK (rss_operation_click), &rss_op_view_article);

		/*
			Manage Feeds
		*/
		manage_feeds_button = gtk_button_new_with_label ("Manage Feeds");
		gtk_widget_set_sensitive (manage_feeds_button, false);

		gautier_rss_ui_app::set_css_class (manage_feeds_button, "button");

		g_signal_connect (manage_feeds_button, "clicked", G_CALLBACK (manage_feeds_click), nullptr);

		gtk_container_add (GTK_CONTAINER (primary_function_buttons), view_article_button);
		gtk_container_add (GTK_CONTAINER (primary_function_buttons), manage_feeds_button);
	}

	/*
		Info Bar
	*/
	info_bar = gtk_label_new ("Status");
	g_object_ref_sink (info_bar);
	gautier_rss_ui_app::set_css_class (info_bar, "feed_status");
	gtk_widget_set_halign (info_bar, GTK_ALIGN_START);

	/*
		RSS Headlines Tab
	*/
	headlines_view = gtk_notebook_new();
	g_object_ref_sink (headlines_view);
	{
		namespace ns_rss_tabs = gautier_rss_win_main_headlines_frame;

		ns_rss_tabs::initialize_headline_view (headlines_view, monitor_width, monitor_height);

		populate_rss_tabs();

		next_notebook_tab_index = 0;

		initialize_ui_threads();
	}

	/*
		Article Frame
	*/
	GtkWidget* article_frame = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gautier_rss_ui_app::set_css_class (article_frame, "article_frame");
	{
		namespace ns = gautier_rss_win_main_article_frame;
		ns::initialize_article_frame (article_frame);
		ns::layout_article_frame (article_frame, header_bar, article_summary, article_details, article_date,
		                          primary_function_buttons,
		                          info_bar);
	}

	/*
		Window Layout
	*/
	layout_pane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
	g_object_ref_sink (layout_pane);
	{
		layout_rss_view (layout_pane, headlines_view, article_frame);

		gtk_container_add (GTK_CONTAINER (window), layout_pane);
	}

	gtk_widget_show_all (window);

	return;
}
