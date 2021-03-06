/* gnomevfs-info.c - Test for get_file_info() for gnome-vfs

   Copyright (C) 2003, Red Hat

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Bastien Nocera <hadess@hadess.net>
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "authentication.c"

static const gchar *
type_to_string (GnomeVFSFileInfo *info)
{
	if ((!info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE)) {
		return "Unknown";
	}

	switch (info->type) {
	case GNOME_VFS_FILE_TYPE_UNKNOWN:
		return "Unknown";
	case GNOME_VFS_FILE_TYPE_REGULAR:
		return "Regular";
	case GNOME_VFS_FILE_TYPE_DIRECTORY:
		return "Directory";
	case GNOME_VFS_FILE_TYPE_SYMBOLIC_LINK:
		return "Symbolic Link";
	case GNOME_VFS_FILE_TYPE_FIFO:
		return "FIFO";
	case GNOME_VFS_FILE_TYPE_SOCKET:
		return "Socket";
	case GNOME_VFS_FILE_TYPE_CHARACTER_DEVICE:
		return "Character device";
	case GNOME_VFS_FILE_TYPE_BLOCK_DEVICE:
		return "Block device";
	default:
		return "???";
	}
}

static void
show_acl (GnomeVFSACL *acl)
{
	GList *ace_list;
	GList *iter;
	
	printf ("ACLs              :\n");
	
	ace_list = gnome_vfs_acl_get_ace_list (acl);
	
	for (iter = ace_list; iter; iter = iter->next) {
		GnomeVFSACLKind  kind;
		GnomeVFSACE     *ace;
		GnomeVFSACLPerm *perms;
		gboolean         negative;
		gboolean         inherit;
		char            *id;
		const char      *kind_str;
		
		ace = GNOME_VFS_ACE (iter->data);
		
		kind = GNOME_VFS_ACL_KIND_NULL;
		inherit = FALSE;
		negative = FALSE;
		id = "";
		perms = NULL;
		
		g_object_get (ace,
		              "kind", &kind, 
		              "id", &id,
		              "permissions", &perms,
		              "negative", &negative,
		              "inherit", &inherit,
		              NULL);
	
		kind_str = gnome_vfs_acl_kind_to_string (kind);
		
		if (kind == GNOME_VFS_ACL_KIND_NULL) {
			continue;	
		}
		
		printf ("                  : %s:%s:", kind_str, id);
		
		if (perms != NULL) {
			GnomeVFSACLPerm *piter;
			const char *pstr;
			
			for (piter = perms; *piter; piter++) {
				pstr = gnome_vfs_acl_perm_to_string (*piter);
				printf ("%s ", pstr);
			}	
		}
		
		if (inherit || negative) {
			if (inherit && negative) {
				printf (" (negative, inherit)");
			} else {
				printf (" (%s)", negative ? "negative" : "inherit");	
			}
		}
		
		printf ("\n");
	}
	
	gnome_vfs_acl_free_ace_list (ace_list);
}

static void
show_file_info (GnomeVFSFileInfo *info, const char *uri)
{
#define FLAG_STRING(info, which)                                \
	(GNOME_VFS_FILE_INFO_##which (info) ? "YES" : "NO")

	printf ("Name              : %s\n", info->name);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_TYPE)
		printf ("Type              : %s\n", type_to_string (info));

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_SYMLINK_NAME && info->symlink_name != NULL)
		printf ("Symlink to        : %s\n", info->symlink_name);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_MIME_TYPE) {
		GnomeVFSMimeApplication *app;

		printf ("MIME type         : %s\n", info->mime_type);
		
		app = gnome_vfs_mime_get_default_application_for_uri (uri, info->mime_type);

		if (app != NULL) {
			printf ("Default app       : %s\n", 
				gnome_vfs_mime_application_get_desktop_id (app));
		}
	}
	
	if (info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT)
		printf ("SELinux Context   : %s\n", info->selinux_context);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_SIZE)
		printf ("Size              : %" GNOME_VFS_SIZE_FORMAT_STR "\n",
				info->size);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_BLOCK_COUNT)
		printf ("Blocks            : %" GNOME_VFS_SIZE_FORMAT_STR "\n",
				info->block_count);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE)
		printf ("I/O block size    : %d\n", info->io_block_size);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_FLAGS) {
		printf ("Local             : %s\n", FLAG_STRING (info, LOCAL));
		printf ("SUID              : %s\n", FLAG_STRING (info, SUID));
		printf ("SGID              : %s\n", FLAG_STRING (info, SGID));
		printf ("Sticky            : %s\n", FLAG_STRING (info, STICKY));        }

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_PERMISSIONS)
		printf ("Permissions       : %04o\n", info->permissions);


	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_LINK_COUNT)
		printf ("Link count        : %d\n", info->link_count);

	if (info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_IDS) {
		printf ("UID               : %d\n", info->uid);
		printf ("GID               : %d\n", info->gid);
	}
	
	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_ATIME)
		printf ("Access time       : %s", ctime (&info->atime));

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_MTIME)
		printf ("Modification time : %s", ctime (&info->mtime));

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_CTIME)
		printf ("Change time       : %s", ctime (&info->ctime));

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_DEVICE)
		printf ("Device #          : %ld\n", (gulong) info->device);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_INODE)
		printf ("Inode #           : %ld\n", (gulong) info->inode);

	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_ACCESS) {
		printf ("Readable          : %s\n",
				(info->permissions&GNOME_VFS_PERM_ACCESS_READABLE?"YES":"NO"));
		printf ("Writable          : %s\n",
				(info->permissions&GNOME_VFS_PERM_ACCESS_WRITABLE?"YES":"NO"));
		printf ("Executable        : %s\n",
				(info->permissions&GNOME_VFS_PERM_ACCESS_EXECUTABLE?"YES":"NO"));
	}
	
	if(info->valid_fields&GNOME_VFS_FILE_INFO_FIELDS_ACL) {
		show_acl (info->acl);	
	}

#undef FLAG_STRING
}

static gboolean slow_mime = FALSE;
static gboolean follow_links = TRUE;

static GOptionEntry entries[] = 
{
	{ "slow-mime", 's', 0, G_OPTION_ARG_NONE, &slow_mime, "force slow MIME type detection where available (sniffing, algorithmic detection, etc)", NULL },
	{ "no-follow", 'n', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &follow_links, "do not automatically follow symbolic links and retrieve the properties of their target", NULL },
	{ NULL }
};



int
main (int argc, char **argv)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult res;
	GnomeVFSFileInfoOptions options;
	char *text_uri;
	GError *error;
	GOptionContext *context;
	
	error = NULL;
	context = g_option_context_new ("- get file info from uri");
  	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  	g_option_context_parse (context, &argc, &argv, &error);
	
	if (argc != 2 || error != NULL) {
		fprintf (stderr, "Usage: %s <uri>\n", argv[0]);
		return 1;
	}

	if (! gnome_vfs_init ()) {
		fprintf (stderr, "Cannot initialize gnome-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	text_uri = gnome_vfs_make_uri_from_shell_arg (argv[1]);
	
	info = gnome_vfs_file_info_new ();
	
	options = GNOME_VFS_FILE_INFO_GET_MIME_TYPE | 
		  GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS |
		  GNOME_VFS_FILE_INFO_GET_ACL |
		  GNOME_VFS_FILE_INFO_GET_SELINUX_CONTEXT;

	if (slow_mime) {
		options |= GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE;
	} else {
		options |= GNOME_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE;
	}

	if (follow_links) {
		options |= GNOME_VFS_FILE_INFO_FOLLOW_LINKS;
	}
	
	res = gnome_vfs_get_file_info (text_uri, info, options);
	
	if (res == GNOME_VFS_OK) {
		show_file_info (info, text_uri);
	} else {
		g_print ("Error: %s\n", gnome_vfs_result_to_string (res));
	}

	g_option_context_free (context);
	gnome_vfs_file_info_unref (info);
	g_free (text_uri);
	
	return 0;
}
