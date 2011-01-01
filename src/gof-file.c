/*
 * Copyright (C) 2010 ammonkey
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: ammonkey <am.monkeyd@gmail.com>
 */

#include "gof-file.h"
#include <stdlib.h>
#include <string.h>
#include "marlin-global-preferences.h" 
#include "eel-i18n.h"
#include "eel-fcts.h"
#include "eel-string.h"
#include "marlin-vala.h"


/*struct _GOFFilePrivate {
  GFileInfo* _file_info;
  };*/


//#define gof_FILE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GOF_TYPE_FILE, GOFFilePrivate))
/*enum  {
  gof_FILE_DUMMY_PROPERTY,
  gof_FILE_NAME,
  gof_FILE_SIZE,
  gof_FILE_DIRECTORY
  };*/

enum {
    FM_LIST_MODEL_FILE_COLUMN,
    FM_LIST_MODEL_ICON,
    FM_LIST_MODEL_COLOR,
    FM_LIST_MODEL_FILENAME,
    FM_LIST_MODEL_SIZE,
    FM_LIST_MODEL_TYPE,
    FM_LIST_MODEL_MODIFIED,
    /*FM_LIST_MODEL_SUBDIRECTORY_COLUMN,
      FM_LIST_MODEL_SMALLEST_ICON_COLUMN,
      FM_LIST_MODEL_SMALLER_ICON_COLUMN,
      FM_LIST_MODEL_SMALL_ICON_COLUMN,
      FM_LIST_MODEL_STANDARD_ICON_COLUMN,
      FM_LIST_MODEL_LARGE_ICON_COLUMN,
      FM_LIST_MODEL_LARGER_ICON_COLUMN,
      FM_LIST_MODEL_LARGEST_ICON_COLUMN,
      FM_LIST_MODEL_SMALLEST_EMBLEM_COLUMN,
      FM_LIST_MODEL_SMALLER_EMBLEM_COLUMN,
      FM_LIST_MODEL_SMALL_EMBLEM_COLUMN,
      FM_LIST_MODEL_STANDARD_EMBLEM_COLUMN,
      FM_LIST_MODEL_LARGE_EMBLEM_COLUMN,
      FM_LIST_MODEL_LARGER_EMBLEM_COLUMN,
      FM_LIST_MODEL_LARGEST_EMBLEM_COLUMN,
      FM_LIST_MODEL_FILE_NAME_IS_EDITABLE_COLUMN,*/
    FM_LIST_MODEL_NUM_COLUMNS
};


//static void gof_file_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
//static void gof_file_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);

G_DEFINE_TYPE (GOFFile, gof_file, G_TYPE_OBJECT)

#define SORT_LAST_CHAR1 '.'
#define SORT_LAST_CHAR2 '#'

#define ICON_NAME_THUMBNAIL_LOADING   "image-loading"

enum {
    //CHANGED,
    //UPDATED_DEEP_COUNT_IN_PROGRESS,
    DESTROY,
    LAST_SIGNAL
};

static guint    signals[LAST_SIGNAL];
static guint32  effective_user_id;

GOFFile* gof_file_new (GFileInfo* file_info, GFile *dir)
{
    GOFFile * self;
    NautilusIconInfo *nicon;

    g_return_val_if_fail (file_info != NULL, NULL);
    self = (GOFFile*) g_object_new (GOF_TYPE_FILE, NULL);
    self->info = file_info;
    //self->parent_dir = g_file_enumerator_get_container (enumerator);
    self->directory = dir;
    //log_printf (LOG_LEVEL_UNDEFINED, "test parent_dir %s\n", g_file_get_uri(self->directory));
    //g_object_ref (self->directory);
    self->name = g_file_info_get_name (file_info);
    self->display_name = g_file_info_get_display_name (file_info);
    self->is_hidden = g_file_info_get_is_hidden (file_info);
    /* don't waste time on collecting data for hidden files which would be dropped */
    if (self->is_hidden && !g_settings_get_boolean(settings, "show-hiddenfiles"))
        return self;
    self->location = g_file_get_child(self->directory, self->name);
    self->ftype = g_file_info_get_attribute_string (file_info, G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE);
    self->size = (guint64) g_file_info_get_size (file_info);
    self->file_type = g_file_info_get_file_type(file_info);
    self->is_directory = (self->file_type == G_FILE_TYPE_DIRECTORY);
    self->modified = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED);

    self->utf8_collation_key = g_utf8_collate_key (self->name, -1);
    self->format_size = g_format_size_for_display(self->size);
    self->formated_modified = gof_file_get_date_as_string (self->modified);
    self->icon = g_content_type_get_icon (self->ftype);

    nicon = nautilus_icon_info_lookup (self->icon, 16);
    self->pix = nautilus_icon_info_get_pixbuf_nodefault (nicon);
    g_object_unref (nicon);

    self->color = NULL;

    return self;
}

GFileInfo* gof_file_get_file_info (GOFFile* self) {
    GFileInfo* result;
    g_return_val_if_fail (self != NULL, NULL);
    result = self->info;
    return result;
}

static void gof_file_init (GOFFile *self) {
    ;
}

static void gof_file_finalize (GObject* obj) {
    GOFFile *file;

    file = GOF_FILE (obj);
    log_printf (LOG_LEVEL_UNDEFINED, "%s %s\n", G_STRFUNC, file->name);
    _g_object_unref0 (file->info);
    _g_object_unref0 (file->location);
    g_free(file->utf8_collation_key);
    g_free(file->format_size);
    g_free(file->formated_modified);
    _g_object_unref0 (file->icon);
    _g_object_unref0 (file->pix);

    G_OBJECT_CLASS (gof_file_parent_class)->finalize (obj);
}

static void gof_file_class_init (GOFFileClass * klass) {

    /* determine the effective user id of the process */
    effective_user_id = geteuid ();

    gof_file_parent_class = g_type_class_peek_parent (klass);
    //g_type_class_add_private (klass, sizeof (GOFFilePrivate));
    /*G_OBJECT_CLASS (klass)->get_property = gof_file_get_property;
      G_OBJECT_CLASS (klass)->set_property = gof_file_set_property;*/
    G_OBJECT_CLASS (klass)->finalize = gof_file_finalize;

    signals[DESTROY] =g_signal_new ("destroy",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (GOFFileClass, destroy),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__VOID,
                                    G_TYPE_NONE, 0);


    /*g_object_class_install_property (G_OBJECT_CLASS (klass), gof_FILE_NAME, g_param_spec_string ("name", "name", "name", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
      g_object_class_install_property (G_OBJECT_CLASS (klass), gof_FILE_SIZE, g_param_spec_uint64 ("size", "size", "size", 0, G_MAXUINT64, 0U, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
      g_object_class_install_property (G_OBJECT_CLASS (klass), gof_FILE_DIRECTORY, g_param_spec_boolean ("directory", "directory", "directory", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));*/
}

/*
   static void gof_file_instance_init (GOFFile * self) {
   self->priv = gof_FILE_GET_PRIVATE (self);
   }*/

#if 0
GType gof_file_get_type (void) {
    static volatile gsize gof_file_type_id__volatile = 0;
    if (g_once_init_enter (&gof_file_type_id__volatile)) {
        static const GTypeInfo g_define_type_info = { sizeof (GOFFileClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gof_file_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GOFFile), 0, (GInstanceInitFunc) gof_file_instance_init, NULL };
        GType gof_file_type_id;
        gof_file_type_id = g_type_register_static (G_TYPE_OBJECT, "GOFFile", &g_define_type_info, 0);
        g_once_init_leave (&gof_file_type_id__volatile, gof_file_type_id);
    }
    return gof_file_type_id__volatile;
}
#endif

#if 0
static void gof_file_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
    GOFFile * self;
    self = GOF_FILE (object);
    switch (property_id) {
    case gof_FILE_NAME:
        g_value_set_string (value, gof_file_get_name (self));
        break;
    case gof_FILE_SIZE:
        g_value_set_uint64 (value, gof_file_get_size (self));
        break;
    case gof_FILE_DIRECTORY:
        g_value_set_boolean (value, gof_file_get_directory (self));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void gof_file_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
    GOFFile * self;
    self = GOF_FILE (object);
    switch (property_id) {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}
#endif

static int
compare_files_by_time (GOFFile *file1, GOFFile *file2)
{
    if (file1->modified < file2->modified)
        return -1;
    else if (file1->modified > file2->modified)
        return 1;

    return 0;
}

static int
compare_by_time (GOFFile *file1, GOFFile *file2)
{
    if (file1->is_directory && !file2->is_directory)
        return -1;
    if (file2->is_directory && !file1->is_directory)
        return 1;

    return compare_files_by_time (file1, file2);
}

static int
compare_by_type (GOFFile *file1, GOFFile *file2)
{
    /* Directories go first. Then, if mime types are identical,
     * don't bother getting strings (for speed). This assumes
     * that the string is dependent entirely on the mime type,
     * which is true now but might not be later.
     */
    if (file1->is_directory && file2->is_directory)
        return 0;
    if (file1->is_directory)
        return -1;
    if (file2->is_directory)
        return +1;
    return (strcmp (file1->utf8_collation_key, file2->utf8_collation_key));
}

static int
compare_by_display_name (GOFFile *file1, GOFFile *file2)
{
    const char *name_1, *name_2;
    gboolean sort_last_1, sort_last_2;
    int compare;

    name_1 = file1->name;
    name_2 = file2->name;

    sort_last_1 = name_1[0] == SORT_LAST_CHAR1 || name_1[0] == SORT_LAST_CHAR2;
    sort_last_2 = name_2[0] == SORT_LAST_CHAR1 || name_2[0] == SORT_LAST_CHAR2;

    if (sort_last_1 && !sort_last_2) {
        compare = +1;
    } else if (!sort_last_1 && sort_last_2) {
        compare = -1;
    } else {
        compare = strcmp (file1->utf8_collation_key, file2->utf8_collation_key);
    }

    return compare;
}

static int
compare_files_by_size (GOFFile *file1, GOFFile *file2)
{
    if (file1->size < file2->size) {
        return -1;
    }
    else if (file1->size > file2->size) {
        return 1;
    }

    return 0;
}

static int
compare_by_size (GOFFile *file1, GOFFile *file2)
{
    if (file1->is_directory && !file2->is_directory)
        return -1;
    if (file2->is_directory && !file1->is_directory)
        return 1;

    return compare_files_by_size (file1, file2);
}

static int
gof_file_compare_for_sort_internal (GOFFile *file1,
                                    GOFFile *file2,
                                    gboolean directories_first,
                                    gboolean reversed)
{
    if (directories_first) {
        if (file1->is_directory && !file2->is_directory) {
            return -1;
        }
        if (file2->is_directory && !file1->is_directory) {
            return 1;
        }
    }

    /*if (file1->details->sort_order < file2->details->sort_order) {
      return reversed ? 1 : -1;
      } else if (file_1->details->sort_order > file_2->details->sort_order) {
      return reversed ? -1 : 1;
      }*/

    return 0;
}

int
gof_file_compare_for_sort (GOFFile *file1,
                           GOFFile *file2,
                           gint sort_type,
                           gboolean directories_first,
                           gboolean reversed)
{
    int result;

    if (file1 == file2) {
        return 0;
    }

    result = gof_file_compare_for_sort_internal (file1, file2, directories_first, reversed);
    //log_printf (LOG_LEVEL_UNDEFINED, "res %d %s %s\n", result, file1->name, file2->name);

    if (result == 0) {
        switch (sort_type) {
        case FM_LIST_MODEL_FILENAME:
            result = compare_by_display_name (file1, file2);
            /*if (result == 0) {
              result = compare_by_directory_name (file_1, file_2);
              }*/
            break;
        case FM_LIST_MODEL_SIZE:
            result = compare_by_size (file1, file2);
            break;
        case FM_LIST_MODEL_TYPE:
            result = compare_by_type (file1, file2);
            break;
        case FM_LIST_MODEL_MODIFIED:
            result = compare_by_time (file1, file2);
            break;
        }

        if (reversed) {
            result = -result;
        }
    }
#if 0
    if (result == 0) {
        switch (sort_type) {
        case NAUTILUS_FILE_SORT_BY_DISPLAY_NAME:
            result = compare_by_display_name (file_1, file_2);
            if (result == 0) {
                result = compare_by_directory_name (file_1, file_2);
            }
            break;
        case NAUTILUS_FILE_SORT_BY_DIRECTORY:
            result = compare_by_full_path (file_1, file_2);
            break;
        case NAUTILUS_FILE_SORT_BY_SIZE:
            /* Compare directory sizes ourselves, then if necessary
             * use GnomeVFS to compare file sizes.
             */
            result = compare_by_size (file_1, file_2);
            if (result == 0) {
                result = compare_by_full_path (file_1, file_2);
            }
            break;
        case NAUTILUS_FILE_SORT_BY_TYPE:
            /* GnomeVFS doesn't know about our special text for certain
             * mime types, so we handle the mime-type sorting ourselves.
             */
            result = compare_by_type (file_1, file_2);
            if (result == 0) {
                result = compare_by_full_path (file_1, file_2);
            }
            break;
        case NAUTILUS_FILE_SORT_BY_MTIME:
            result = compare_by_time (file_1, file_2, NAUTILUS_DATE_TYPE_MODIFIED);
            if (result == 0) {
                result = compare_by_full_path (file_1, file_2);
            }
            break;
        case NAUTILUS_FILE_SORT_BY_ATIME:
            result = compare_by_time (file_1, file_2, NAUTILUS_DATE_TYPE_ACCESSED);
            if (result == 0) {
                result = compare_by_full_path (file_1, file_2);
            }
            break;
        case NAUTILUS_FILE_SORT_BY_TRASHED_TIME:
            result = compare_by_time (file_1, file_2, NAUTILUS_DATE_TYPE_TRASHED);
            if (result == 0) {
                result = compare_by_full_path (file_1, file_2);
            }
            break;
        case NAUTILUS_FILE_SORT_BY_EMBLEMS:
            /* GnomeVFS doesn't know squat about our emblems, so
             * we handle comparing them here, before falling back
             * to tie-breakers.
             */
            result = compare_by_emblems (file_1, file_2);
            if (result == 0) {
                result = compare_by_full_path (file_1, file_2);
            }
            break;
        default:
            g_return_val_if_reached (0);
        }

        if (reversed) {
            result = -result;
        }
    }
#endif
    return result;
}

GOFFile *
gof_file_ref (GOFFile *file)
{
    if (file == NULL) {
        return NULL;
    }
    g_return_val_if_fail (GOF_IS_FILE (file), NULL);

    return g_object_ref (file);
}

void
gof_file_unref (GOFFile *file)
{
    if (file == NULL) {
        return;
    }

    g_return_if_fail (GOF_IS_FILE (file));

    g_object_unref (file);
}

static const char *TODAY_TIME_FORMATS [] = {
    /* Today, use special word.
     * strftime patterns preceeded with the widest
     * possible resulting string for that pattern.
     *
     * Note to localizers: You can look at man strftime
     * for details on the format, but you should only use
     * the specifiers from the C standard, not extensions.
     * These include "%" followed by one of
     * "aAbBcdHIjmMpSUwWxXyYZ". There are two extensions
     * in the Nautilus version of strftime that can be
     * used (and match GNU extensions). Putting a "-"
     * between the "%" and any numeric directive will turn
     * off zero padding, and putting a "_" there will use
     * space padding instead of zero padding.
     */
    N_("today at 00:00:00 PM"),
    N_("today at %-I:%M:%S %p"),

    N_("today at 00:00 PM"),
    N_("today at %-I:%M %p"),

    N_("today, 00:00 PM"),
    N_("today, %-I:%M %p"),

    N_("today"),
    N_("today"),

    NULL
};

static const char *YESTERDAY_TIME_FORMATS [] = {
    /* Yesterday, use special word.
     * Note to localizers: Same issues as "today" string.
     */
    N_("yesterday at 00:00:00 PM"),
    N_("yesterday at %-I:%M:%S %p"),

    N_("yesterday at 00:00 PM"),
    N_("yesterday at %-I:%M %p"),

    N_("yesterday, 00:00 PM"),
    N_("yesterday, %-I:%M %p"),

    N_("yesterday"),
    N_("yesterday"),

    NULL
};

static const char *CURRENT_WEEK_TIME_FORMATS [] = {
    /* Current week, include day of week.
     * Note to localizers: Same issues as "today" string.
     * The width measurement templates correspond to
     * the day/month name with the most letters.
     */
    N_("Wednesday, September 00 0000 at 00:00:00 PM"),
    N_("%A, %B %-d %Y at %-I:%M:%S %p"),

    N_("Mon, Oct 00 0000 at 00:00:00 PM"),
    N_("%a, %b %-d %Y at %-I:%M:%S %p"),

    N_("Mon, Oct 00 0000 at 00:00 PM"),
    N_("%a, %b %-d %Y at %-I:%M %p"),

    N_("Oct 00 0000 at 00:00 PM"),
    N_("%b %-d %Y at %-I:%M %p"),

    N_("Oct 00 0000, 00:00 PM"),
    N_("%b %-d %Y, %-I:%M %p"),

    N_("00/00/00, 00:00 PM"),
    N_("%m/%-d/%y, %-I:%M %p"),

    N_("00/00/00"),
    N_("%m/%d/%y"),

    NULL
};

char *
gof_file_get_date_as_string (guint64 d)
{
    //time_t file_time_raw;
    struct tm *file_time;
    const char **formats;
    const char *width_template;
    const char *format;
    char *date_string;
    //char *result;
    GDate *today;
    GDate *file_date;
    guint32 file_date_age;
    int i;

    file_time = localtime (&d);

    gchar *date_format_pref = g_settings_get_string(settings, MARLIN_PREFERENCES_DATE_FORMAT);

    if (!strcmp (date_format_pref, "locale"))
        return eel_strdup_strftime ("%c", file_time);
    else if (!strcmp (date_format_pref, "iso"))
        return eel_strdup_strftime ("%Y-%m-%d %H:%M:%S", file_time);

    file_date = eel_g_date_new_tm (file_time);

    today = g_date_new ();
    g_date_set_time_t (today, time (NULL));

    /* Overflow results in a large number; fine for our purposes. */
    file_date_age = (g_date_get_julian (today) -
                     g_date_get_julian (file_date));

    g_date_free (file_date);
    g_date_free (today);

    /* Format varies depending on how old the date is. This minimizes
     * the length (and thus clutter & complication) of typical dates
     * while providing sufficient detail for recent dates to make
     * them maximally understandable at a glance. Keep all format
     * strings separate rather than combining bits & pieces for
     * internationalization's sake.
     */

    if (file_date_age == 0)	{
        formats = TODAY_TIME_FORMATS;
    } else if (file_date_age == 1) {
        formats = YESTERDAY_TIME_FORMATS;
    } else if (file_date_age < 7) {
        formats = CURRENT_WEEK_TIME_FORMATS;
    } else {
        formats = CURRENT_WEEK_TIME_FORMATS;
    }

    /* Find the date format that just fits the required width. Instead of measuring
     * the resulting string width directly, measure the width of a template that represents
     * the widest possible version of a date in a given format. This is done by using M, m
     * and 0 for the variable letters/digits respectively.
     */
    format = NULL;

    for (i = 0; ; i += 2) {
        width_template = (formats [i] ? _(formats [i]) : NULL);
        if (width_template == NULL) {
            /* no more formats left */
            g_assert (format != NULL);

            /* Can't fit even the shortest format -- return an ellipsized form in the
             * shortest format
             */

            date_string = eel_strdup_strftime (format, file_time);

            return date_string;
        }

        format = _(formats [i + 1]);

        /* don't care about fitting the width */
        break;
    }

    return eel_strdup_strftime (format, file_time);
}

/*
   self->icon = g_content_type_get_icon (self->ftype);

   nicon = nautilus_icon_info_lookup (self->icon, 16);
   self->pix = nautilus_icon_info_get_pixbuf_nodefault (nicon);
   g_object_unref (nicon);
   */

NautilusIconInfo *
gof_file_get_icon (GOFFile *file, int size, GOFFileIconFlags flags)
{
    NautilusIconInfo *icon;
    GIcon *gicon;
    //GdkPixbuf *raw_pixbuf, *scaled_pixbuf;
    //int modified_size;

    if (file == NULL) {
        return NULL;
    }

    /*gicon = get_custom_icon (file);
      if (gicon) {
      icon = nautilus_icon_info_lookup (gicon, size);
      g_object_unref (gicon);
      return icon;
      }*/

#if 0
    if (flags & NAUTILUS_FILE_ICON_FLAGS_USE_THUMBNAILS &&
        nautilus_file_should_show_thumbnail (file)) {
        if (file->details->thumbnail) {
            int w, h, s;
            double scale;

            /*scaled_pixbuf = gdk_pixbuf_scale_simple (raw_pixbuf,
              w * scale, h * scale,
              GDK_INTERP_BILINEAR);*/

            /* We don't want frames around small icons */
            /*if (!gdk_pixbuf_get_has_alpha(raw_pixbuf) || s >= 128) {
              nautilus_thumbnail_frame_image (&scaled_pixbuf);
              }*/

            /*icon = nautilus_icon_info_new_for_pixbuf (scaled_pixbuf);
              g_object_unref (scaled_pixbuf);
              return icon;*/
            /*} else if (file->details->thumbnail_path == NULL &&
              file->details->can_read &&				
              !file->details->is_thumbnailing &&
              !file->details->thumbnailing_failed) {
              if (nautilus_can_thumbnail (file)) {
              nautilus_create_thumbnail (file);
              }
              }*/
    }
#endif
    if (flags & GOF_FILE_ICON_FLAGS_USE_THUMBNAILS) {
        if (file->thumbnail) {
            printf("show thumb\n");
        } else {
            //TODO implement thumb generaton here.
            printf ("if can thumbnail gen thumb\n");
        }
    }

    if (flags & GOF_FILE_ICON_FLAGS_USE_THUMBNAILS
        && file->is_thumbnailing)
        gicon = g_themed_icon_new (ICON_NAME_THUMBNAIL_LOADING);
    else
        gicon = file->icon;

    if (gicon) {
        icon = nautilus_icon_info_lookup (gicon, size);
        if (nautilus_icon_info_is_fallback(icon)) {
            g_object_unref (icon);
            icon = nautilus_icon_info_lookup (g_themed_icon_new ("text-x-generic"), size);
        }
        g_object_unref (gicon);
        return icon;
    } else {
        return nautilus_icon_info_lookup (g_themed_icon_new ("text-x-generic"), size);
    }
}

GdkPixbuf *
gof_file_get_icon_pixbuf (GOFFile *file, int size, gboolean force_size, GOFFileIconFlags flags)
{
    NautilusIconInfo *nicon;
    GdkPixbuf *pix;

    nicon = gof_file_get_icon (file, size, flags);
    if (force_size) {
        pix =  nautilus_icon_info_get_pixbuf_at_size (nicon, size);
    } else {
        pix = nautilus_icon_info_get_pixbuf_nodefault (nicon);
    }
    g_object_unref (nicon);

    return pix;
}

//TODO waiting to be implemented with directories: never create twice the same objects
#if 0
static GOFFile *
gof_file_get_internal (GFile *location, gboolean create)
{
    gboolean self_owned;
    NautilusDirectory *directory;
    NautilusFile *file;
    GFile *parent;
    char *basename;

    g_assert (location != NULL);

    parent = g_file_get_parent (location);

    self_owned = FALSE;
    if (parent == NULL) {
        self_owned = TRUE;
        parent = g_object_ref (location);
    } 

    /* Get object that represents the directory. */
    directory = nautilus_directory_get_internal (parent, create);

    g_object_unref (parent);

    /* Get the name for the file. */
    if (self_owned && directory != NULL) {
        basename = nautilus_directory_get_name_for_self_as_new_file (directory);
    } else {
        basename = g_file_get_basename (location);
    }
    /* Check to see if it's a file that's already known. */
    if (directory == NULL) {
        file = NULL;
    } else if (self_owned) {
        file = directory->details->as_file;
    } else {
        file = nautilus_directory_find_file_by_name (directory, basename);
    }

    /* Ref or create the file. */
    if (file != NULL) {
        nautilus_file_ref (file);
    } else if (create) {
        file = nautilus_file_new_from_filename (directory, basename, self_owned);
        if (self_owned) {
            g_assert (directory->details->as_file == NULL);
            directory->details->as_file = file;
        } else {
            nautilus_directory_add_file (directory, file);
        }
    }

    g_free (basename);
    nautilus_directory_unref (directory);

    return file;
}

GOFFile *
gof_file_get (GFile *location)
{
    return gof_file_get_internal (location, TRUE);
}
#endif

GOFFile* gof_file_get (GFile *location)
{
    /* TODO check directories in a hastable for a already known file */
    /* FIXME this function is a temporary workarround - must be async - TODO reshape the GOFFile */
    GFileInfo *file_info;
    GFile *parent;
    GOFFile *file;

    parent = g_file_get_parent (location);
    file_info = g_file_query_info (location, GOF_GIO_DEFAULT_ATTRIBUTES,
                                   G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
    if (file_info == NULL)
        return NULL;
    file = gof_file_new(file_info, parent);

    return (file);
}

GOFFile* gof_file_get_by_uri (const char *uri)
{
    GFile *location;
    GOFFile *file;
	
    location = g_file_new_for_uri (uri);
    file = gof_file_get (location);
    g_object_unref (location);
	
    return file;
}

/* Return value: the string representation of @list conforming to the
 *               text/uri-list mime type defined in RFC 2483.
 */
#if 0
gchar *
gof_g_file_list_to_string (GList *list, gsize *len)
{
    GString *string;
    gchar   *uri;
    GList   *lp;

    /* allocate initial string */
    string = g_string_new (NULL);

    for (lp = list; lp != NULL; lp = lp->next)
    {
        uri = g_file_get_uri (lp->data);
        string = g_string_append (string, uri);
        g_free (uri);

        string = g_string_append (string, "\r\n");
    }

    *len = string->len;
    return g_string_free (string, FALSE);
}
#endif

gchar *
gof_file_list_to_string (GList *list, gsize *len)
{
    GString *string;
    gchar   *uri;
    GList   *lp;

    /* allocate initial string */
    string = g_string_new (NULL);

    for (lp = list; lp != NULL; lp = lp->next)
    {
        uri = g_file_get_uri (GOF_FILE(lp->data)->location);
        string = g_string_append (string, uri);
        g_free (uri);

        string = g_string_append (string, "\r\n");
    }

    *len = string->len;
    return g_string_free (string, FALSE); 
}

gboolean
gof_file_same_filesystem (GOFFile *file_a, GOFFile *file_b)
{
    const gchar *filesystem_id_a;
    const gchar *filesystem_id_b;

    g_return_val_if_fail (GOF_IS_FILE (file_a), FALSE);
    g_return_val_if_fail (GOF_IS_FILE (file_b), FALSE);

    /* return false if we have no information about one of the files */
    if (file_a->info == NULL || file_b->info == NULL)
        return FALSE;

    /* determine the filesystem IDs */
    filesystem_id_a = g_file_info_get_attribute_string (file_a->info, 
                                                        G_FILE_ATTRIBUTE_ID_FILESYSTEM);

    filesystem_id_b = g_file_info_get_attribute_string (file_b->info, 
                                                        G_FILE_ATTRIBUTE_ID_FILESYSTEM);

    /* compare the filesystem IDs */
    return eel_str_is_equal (filesystem_id_a, filesystem_id_b);
}

/**
 * gof_file_accepts_drop (imported from thunar):
 * @file                    : a #GOFFile instance.
 * @file_list               : the list of #GFile<!---->s that will be droppped.
 * @context                 : the current #GdkDragContext, which is used for the drop.
 * @suggested_action_return : return location for the suggested #GdkDragAction or %NULL.
 *
 * Checks whether @file can accept @path_list for the given @context and
 * returns the #GdkDragAction<!---->s that can be used or 0 if no actions
 * apply.
 *
 * If any #GdkDragAction<!---->s apply and @suggested_action_return is not
 * %NULL, the suggested #GdkDragAction for this drop will be stored to the
 * location pointed to by @suggested_action_return.
 *
 * Return value: the #GdkDragAction<!---->s supported for the drop or
 *               0 if no drop is possible.
**/

GdkDragAction
gof_file_accepts_drop (GOFFile          *file,
                       GList            *file_list,
                       GdkDragContext   *context,
                       GdkDragAction    *suggested_action_return)
{
    GdkDragAction   suggested_action;
    GdkDragAction   actions;
    GOFFile         *ofile;
    GFile           *parent_file;
    GList           *lp;
    guint           n;

    g_return_val_if_fail (GOF_IS_FILE (file), 0);
    g_return_val_if_fail (GDK_IS_DRAG_CONTEXT (context), 0);

    /* we can never drop an empty list */
    if (G_UNLIKELY (file_list == NULL))
        return 0;

    /* default to whatever GTK+ thinks for the suggested action */
    suggested_action = gdk_drag_context_get_suggested_action (context);

    printf ("%s %s %s\n", G_STRFUNC, g_file_get_uri (file->location), g_file_get_uri (file_list->data));
    /* check if we have a writable directory here or an executable file */
    if (file->is_directory)
        //TODO
        //&& thunar_file_is_writable (file))
    {
        /* determine the possible actions */
        actions = gdk_drag_context_get_actions (context) & (GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_ASK);

        /* cannot create symbolic links in the trash or copy to the trash */
        //TODO
        /*if (thunar_file_is_trashed (file))
          actions &= ~(GDK_ACTION_COPY | GDK_ACTION_LINK);*/

        /* check up to 100 of the paths (just in case somebody tries to
         * drag around his music collection with 5000 files).
         */
        for (lp = file_list, n = 0; lp != NULL && n < 100; lp = lp->next, ++n)
        {
            printf ("%s %s %s\n", G_STRFUNC, g_file_get_uri (file->location), g_file_get_uri (lp->data));
            /* we cannot drop a file on itself */
            if (G_UNLIKELY (g_file_equal (file->location, lp->data)))
                return 0;

            /* check whether source and destination are the same */
            parent_file = g_file_get_parent (lp->data);
            if (G_LIKELY (parent_file != NULL))
            {
                if (g_file_equal (file->location, parent_file))
                {
                    g_object_unref (parent_file);
                    return 0;
                }
                else
                    g_object_unref (parent_file);
            }

            /* copy/move/link within the trash not possible */
            //TODO
            /*if (G_UNLIKELY (thunar_g_file_is_trashed (lp->data) && thunar_file_is_trashed (file)))
              return 0;*/
        }

        /* if the source offers both copy and move and the GTK+ suggested action is copy, try to be smart telling whether we should copy or move by default by checking whether the source and target are on the same disk. */
        if ((actions & (GDK_ACTION_COPY | GDK_ACTION_MOVE)) != 0 
            && (suggested_action == GDK_ACTION_COPY))
        {
            /* default to move as suggested action */
            suggested_action = GDK_ACTION_MOVE;

            /* check for up to 100 files, for the reason state above */
            for (lp = file_list, n = 0; lp != NULL && n < 100; lp = lp->next, ++n)
            {
                /* dropping from the trash always suggests move */
                //TODO
                /*if (G_UNLIKELY (thunar_g_file_is_trashed (lp->data)))
                  break;*/

                /* determine the cached version of the source file */
                //ofile = thunar_file_cache_lookup (lp->data);
                //ofile = NULL;
                ofile = gof_file_get(lp->data);

                /* we have only move if we know the source and both the source and the target
                 * are on the same disk, and the source file is owned by the current user.
                 */
                if (ofile == NULL 
                    || !gof_file_same_filesystem (file, ofile)
                    || (ofile->info != NULL 
                        && g_file_info_get_attribute_uint32 (ofile->info, 
                                                             G_FILE_ATTRIBUTE_UNIX_UID) != effective_user_id))
                {
                    //printf ("%s default suggested action GDK_ACTION_COPY\n", G_STRFUNC);
                    /* default to copy and get outa here */
                    suggested_action = GDK_ACTION_COPY;
                    break;
                }
            }
            //printf ("%s actions MOVE %d COPY %d suggested %d\n", G_STRFUNC, GDK_ACTION_MOVE, GDK_ACTION_COPY, suggested_action);
        }
    }
    //TODO
#if 0
    else if (thunar_file_is_executable (file))
    {
        /* determine the possible actions */
        actions = gdk_drag_context_get_actions (context) & (GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK | GDK_ACTION_PRIVATE);
    }
#endif
    else
        return 0;

    /* determine the preferred action based on the context */
    if (G_LIKELY (suggested_action_return != NULL))
    {
        /* determine a working action */
        if (G_LIKELY ((suggested_action & actions) != 0))
            *suggested_action_return = suggested_action;
        else if ((actions & GDK_ACTION_ASK) != 0)
            *suggested_action_return = GDK_ACTION_ASK;
        else if ((actions & GDK_ACTION_COPY) != 0)
            *suggested_action_return = GDK_ACTION_COPY;
        else if ((actions & GDK_ACTION_LINK) != 0)
            *suggested_action_return = GDK_ACTION_LINK;
        else if ((actions & GDK_ACTION_MOVE) != 0)
            *suggested_action_return = GDK_ACTION_MOVE;
        else
            *suggested_action_return = GDK_ACTION_PRIVATE;
    }

    /* yeppa, we can drop here */
    return actions;
}

void
gof_file_list_unref (GList *list)
{
    g_list_foreach (list, (GFunc) gof_file_unref, NULL);
}

void
gof_file_list_free (GList *list)
{
    gof_file_list_unref (list);
    g_list_free (list);
}

