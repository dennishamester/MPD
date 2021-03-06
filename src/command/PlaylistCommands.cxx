/*
 * Copyright (C) 2003-2015 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "PlaylistCommands.hxx"
#include "Request.hxx"
#include "db/DatabasePlaylist.hxx"
#include "CommandError.hxx"
#include "PlaylistPrint.hxx"
#include "PlaylistSave.hxx"
#include "PlaylistFile.hxx"
#include "db/PlaylistVector.hxx"
#include "SongLoader.hxx"
#include "BulkEdit.hxx"
#include "playlist/PlaylistQueue.hxx"
#include "playlist/Print.hxx"
#include "queue/Playlist.hxx"
#include "TimePrint.hxx"
#include "client/Client.hxx"
#include "client/Response.hxx"
#include "ls.hxx"
#include "Mapper.hxx"
#include "fs/AllocatedPath.hxx"
#include "util/UriUtil.hxx"
#include "util/Error.hxx"
#include "util/ConstBuffer.hxx"

bool
playlist_commands_available()
{
	return !map_spl_path().IsNull();
}

static void
print_spl_list(Response &r, const PlaylistVector &list)
{
	for (const auto &i : list) {
		r.Format("playlist: %s\n", i.name.c_str());

		if (i.mtime > 0)
			time_print(r, "Last-Modified", i.mtime);
	}
}

CommandResult
handle_save(Client &client, Request args, Response &r)
{
	Error error;
	return spl_save_playlist(args.front(), client.playlist, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_load(Client &client, Request args, Response &r)
{
	RangeArg range = RangeArg::All();
	if (!args.ParseOptional(1, range, r))
		return CommandResult::ERROR;

	const ScopeBulkEdit bulk_edit(client.partition);

	Error error;
	const SongLoader loader(client);
	if (!playlist_open_into_queue(args.front(),
				      range.start, range.end,
				      client.playlist,
				      client.player_control, loader, error))
		return print_error(r, error);

	return CommandResult::OK;
}

CommandResult
handle_listplaylist(Client &client, Request args, Response &r)
{
	const char *const name = args.front();

	if (playlist_file_print(r, client.partition, SongLoader(client),
				name, false))
		return CommandResult::OK;

	Error error;
	return spl_print(r, client.partition, name, false, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_listplaylistinfo(Client &client, Request args, Response &r)
{
	const char *const name = args.front();

	if (playlist_file_print(r, client.partition, SongLoader(client),
				name, true))
		return CommandResult::OK;

	Error error;
	return spl_print(r, client.partition, name, true, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_rm(gcc_unused Client &client, Request args, Response &r)
{
	const char *const name = args.front();

	Error error;
	return spl_delete(name, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_rename(gcc_unused Client &client, Request args, Response &r)
{
	const char *const old_name = args[0];
	const char *const new_name = args[1];

	Error error;
	return spl_rename(old_name, new_name, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_playlistdelete(gcc_unused Client &client, Request args, Response &r)
{
	const char *const name = args[0];
	unsigned from;
	if (!args.Parse(1, from, r))
		return CommandResult::ERROR;

	Error error;
	return spl_remove_index(name, from, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_playlistmove(gcc_unused Client &client, Request args, Response &r)
{
	const char *const name = args.front();
	unsigned from, to;
	if (!args.Parse(1, from, r) || !args.Parse(2, to, r))
		return CommandResult::ERROR;

	Error error;
	return spl_move_index(name, from, to, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_playlistclear(gcc_unused Client &client, Request args, Response &r)
{
	const char *const name = args.front();

	Error error;
	return spl_clear(name, error)
		? CommandResult::OK
		: print_error(r, error);
}

CommandResult
handle_playlistadd(Client &client, Request args, Response &r)
{
	const char *const playlist = args[0];
	const char *const uri = args[1];

	bool success;
	Error error;
	if (uri_has_scheme(uri)) {
		const SongLoader loader(client);
		success = spl_append_uri(playlist, loader, uri, error);
	} else {
#ifdef ENABLE_DATABASE
		const Database *db = client.GetDatabase(error);
		if (db == nullptr)
			return print_error(r, error);

		success = search_add_to_playlist(*db, *client.GetStorage(),
						 uri, playlist, nullptr,
						 error);
#else
		success = false;
#endif
	}

	if (!success && !error.IsDefined()) {
		r.Error(ACK_ERROR_NO_EXIST, "directory or file not found");
		return CommandResult::ERROR;
	}

	return success ? CommandResult::OK : print_error(r, error);
}

CommandResult
handle_listplaylists(gcc_unused Client &client, gcc_unused Request args,
		     Response &r)
{
	Error error;
	const auto list = ListPlaylistFiles(error);
	if (list.empty() && error.IsDefined())
		return print_error(r, error);

	print_spl_list(r, list);
	return CommandResult::OK;
}
