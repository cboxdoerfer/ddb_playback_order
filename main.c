/*
    Playback Order Plugin for DeaDBeeF audio player
    Copyright (C) 2014 Christian Boxdörfer <christian.boxdoerfer@posteo.de>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>
#include <deadbeef/gtkui_api.h>

#include "support.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_misc_t plugin;
static DB_functions_t *deadbeef = NULL;
static ddb_gtkui_t *gtkui_plugin = NULL;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *combo_box;
} w_playback_order_t;

static void
playback_order_set_combo_box (GtkWidget *widget) {
    if (!widget) {
        return;
    }
    int ord = deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    switch (ord) {
    case PLAYBACK_ORDER_LINEAR:
    case PLAYBACK_ORDER_SHUFFLE_TRACKS:
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), ord);
        break;
    case PLAYBACK_ORDER_SHUFFLE_ALBUMS:
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), PLAYBACK_ORDER_RANDOM);
        break;
    case PLAYBACK_ORDER_RANDOM:
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), PLAYBACK_ORDER_SHUFFLE_ALBUMS);
        break;
    default:
        gtk_combo_box_set_active (GTK_COMBO_BOX (widget), PLAYBACK_ORDER_LINEAR);
        break;
    }
}
static int
playback_order_message (ddb_gtkui_widget_t *widget, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2)
{
    w_playback_order_t *w = (w_playback_order_t *)widget;
    switch (id) {
        case DB_EV_CONFIGCHANGED:
            playback_order_set_combo_box (w->combo_box);
            break;
    }
    return 0;
}

static void
playback_order_changed (GtkComboBox *widget, gpointer user_data)
{
    int playback_order = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
    switch (playback_order) {
    case PLAYBACK_ORDER_LINEAR:
    case PLAYBACK_ORDER_SHUFFLE_TRACKS:
        break;
    case PLAYBACK_ORDER_SHUFFLE_ALBUMS:
        playback_order = PLAYBACK_ORDER_RANDOM;
        break;
    case PLAYBACK_ORDER_RANDOM:
        playback_order = PLAYBACK_ORDER_SHUFFLE_ALBUMS;
        break;
    default:
        playback_order = PLAYBACK_ORDER_LINEAR;
        break;
    }
    int playback_order_old = deadbeef->conf_get_int ("playback.order", PLAYBACK_ORDER_LINEAR);
    if (playback_order != playback_order_old) {
        deadbeef->conf_set_int ("playback.order", playback_order);
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }
}

static void
playback_order_init (ddb_gtkui_widget_t *ww) {
    w_playback_order_t *w = (w_playback_order_t *)ww;

    GtkWidget *hbox = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (w->base.widget), hbox);

    GtkWidget *label = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (label),"Order:");
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);

    w->combo_box = gtk_combo_box_text_new ();
    gtk_widget_show (w->combo_box);
    gtk_box_pack_start (GTK_BOX (hbox), w->combo_box, TRUE, TRUE, 0);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(w->combo_box), "Linear");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(w->combo_box), "Shuffle Tracks");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(w->combo_box), "Shuffle Albums");
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(w->combo_box), "Random");

    playback_order_set_combo_box (w->combo_box);

    g_signal_connect_after ((gpointer) w->combo_box, "changed", G_CALLBACK (playback_order_changed), w);

    gtkui_plugin->w_override_signals (w->combo_box, w);
}

static void
playback_order_destroy (ddb_gtkui_widget_t *w) {
}

static ddb_gtkui_widget_t *
w_playback_order_create (void) {
    w_playback_order_t *w = malloc (sizeof (w_playback_order_t));
    memset (w, 0, sizeof (w_playback_order_t));

    w->base.widget = gtk_event_box_new ();
    w->base.destroy  = playback_order_destroy;
    w->base.message = playback_order_message;
    w->base.init = playback_order_init;
    gtkui_plugin->w_override_signals (w->base.widget, w);

    return (ddb_gtkui_widget_t *)w;
}

static int
playback_order_connect (void)
{
    gtkui_plugin = (ddb_gtkui_t *) deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if (gtkui_plugin) {
        //trace("using '%s' plugin %d.%d\n", DDB_GTKUI_PLUGIN_ID, gtkui_plugin->gui.plugin.version_major, gtkui_plugin->gui.plugin.version_minor );
        if (gtkui_plugin->gui.plugin.version_major == 2) {
            //printf ("fb api2\n");
            // 0.6+, use the new widget API
            gtkui_plugin->w_reg_widget ("Playback order", DDB_WF_SINGLE_INSTANCE, w_playback_order_create, "playback_order", NULL);
            return 0;
        }
    }
    return -1;
}

static int
playback_order_disconnect (void)
{
    gtkui_plugin = NULL;
    return 0;
}

static int
playback_order_start (void)
{
    //load_config ();
    return 0;
}

static int
playback_order_stop (void)
{
    trace ("bookmark_stop\n");
    return 0;
}

// define plugin interface
static DB_misc_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 5,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "playback_order_widget-gtk3",
#else
    .plugin.id = "playback_order_widget",
#endif
    .plugin.name = "Playback order menu",
    .plugin.descr = "A dropdown menu to switch between different playback orders",
    .plugin.copyright =
        "Copyright (C) 2015 Christian Boxdörfer <christian.boxdoerfer@posteo.de>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .plugin.website = "http://www.github.com/cboxdoerfer/ddb_playback_order",
    .plugin.start = playback_order_start,
    .plugin.stop = playback_order_stop,
    .plugin.connect  = playback_order_connect,
    .plugin.disconnect  = playback_order_disconnect,
};

#if !GTK_CHECK_VERSION(3,0,0)
DB_plugin_t *
ddb_misc_playback_order_GTK2_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return &plugin.plugin;
}
#else
DB_plugin_t *
ddb_misc_playback_order_GTK3_load (DB_functions_t *ddb) {
    deadbeef = ddb;
    return &plugin.plugin;
}
#endif
