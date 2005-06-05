/* 
 * gnc-plugin-page-register.c -- 
 *
 * Copyright (C) 2003 Jan Arne Petersen <jpetersen@uni-bonn.de>
 * Copyright (C) 2003 David Hampton <hampton@employees.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact:
 *
 * Free Software Foundation           Voice:  +1-617-542-5942
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652
 * Boston, MA  02111-1307,  USA       gnu@gnu.org
 */

#include "config.h"

#include <gtk/gtk.h>
#include <g-wrap-wct.h>

#include "gnc-plugin-page-register.h"
#include "gnc-plugin-register.h"
#include "gnc-plugin-menu-additions.h"
#include "gnc-plugin-page-report.h"

#include "dialog-print-check.h"
#include "dialog-transfer.h"
#include "dialog-utils.h"
#include "druid-stock-split.h"
#include "global-options.h"
#include "gnc-book.h"
#include "gnc-date.h"
#include "gnc-date-edit.h"
#include "gnc-gnome-utils.h"
#include "gnc-icons.h"
#include "gnc-split-reg.h"
#include "gnc-ui-util.h"
#include "gnc-window.h"
#include "lot-viewer.h"
#include "QueryNew.h"
#include "window-reconcile.h"
#include "window-report.h"

#include "messages.h"
#include "gnc-engine-util.h"

/* This static indicates the debugging module that this .o belongs to.  */
static short module = MOD_GUI;
static GList *active_pages = NULL;

#define DEFAULT_LINES_OPTION_PAGE "_+Advanced"
#define DEFAULT_LINES_OPTION_NAME "Number of Rows"
#define DEFAULT_LINES_AMOUNT      20

static void gnc_plugin_page_register_class_init (GncPluginPageRegisterClass *klass);
static void gnc_plugin_page_register_init (GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_finalize (GObject *object);

/* static Account *gnc_plugin_page_register_get_current_account (GncPluginPageRegister *page); */

static GtkWidget *gnc_plugin_page_register_create_widget (GncPluginPage *plugin_page);
static void gnc_plugin_page_register_destroy_widget (GncPluginPage *plugin_page);
static void gnc_plugin_page_register_merge_actions (GncPluginPage *plugin_page, GtkUIManager *ui_merge);
static void gnc_plugin_page_register_unmerge_actions (GncPluginPage *plugin_page, GtkUIManager *ui_merge);

static gchar *gnc_plugin_page_register_get_tab_name (GncPluginPage *plugin_page);

/* Callbacks */
static gboolean gnc_plugin_page_register_button_press_cb (GtkWidget *widget,
							      GdkEventButton *event,
			       				      GncPluginPageRegister *page);

void gnc_plugin_page_register_start_toggle_cb(GtkToggleButton *toggle, gpointer data);
void gnc_plugin_page_register_end_toggle_cb(GtkToggleButton *toggle, gpointer data);
void gnc_plugin_page_register_today_cb(GtkButton *buttontoggle, gpointer data);

/* Command callbacks */
static void gnc_plugin_page_register_cmd_print_check (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_date_show_all (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_date_set_range (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_status_all (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_status_one (GtkToggleAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_sort_changed (GtkAction *action, GtkRadioAction *current, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_style_changed (GtkAction *action, GtkRadioAction *current, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_style_double_line (GtkToggleAction *action, GncPluginPageRegister *plugin_page);

static void gnc_plugin_page_register_cmd_reconcile (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_transfer (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_stock_split (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_lots (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_enter_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_cancel_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_delete_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_blank_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_duplicate_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_reinitialize_transaction (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_expand_transaction (GtkToggleAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_exchange_rate (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_jump (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_schedule (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_account_report (GtkAction *action, GncPluginPageRegister *plugin_page);
static void gnc_plugin_page_register_cmd_transaction_report (GtkAction *action, GncPluginPageRegister *plugin_page);

static void gnc_plugin_page_help_changed_cb( GNCSplitReg *gsr, GncPluginPageRegister *register_page );

/************************************************************/
/*                          Actions                         */
/************************************************************/

static GtkActionEntry gnc_plugin_page_register_actions [] =
{
	/* Toplevel */
	{ "FakeToplevel", NULL, "", NULL, NULL, NULL },
	{ "SelectTransactionsAction", NULL, N_("Select _Transactions"), NULL, NULL, NULL },
	{ "TransDateRangeAction", NULL, N_("_Date Range"), NULL, NULL, NULL },
	{ "TransStatusAction", NULL, N_("_Status"), NULL, NULL, NULL },
	{ "SortOrderAction", NULL, N_("Sort _Order"), NULL, NULL, NULL },
	{ "StyleAction", NULL, N_("St_yle"), NULL, NULL, NULL },

	/* File menu */
	{ "FilePrintCheckAction", NULL, N_("Print Check"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_print_check) },

	/* View menu */
	{ "TransDateShowAllAction", NULL, N_("Show _All"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_date_show_all) },
	{ "TransDateSetRangeAction", NULL, N_("Set _Range..."), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_date_set_range) },

	{ "StatusAllAction", NULL, N_("_All"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_status_all) },

	/* Actions menu */
	{ "ActionsTransferAction", GTK_STOCK_MISSING_IMAGE, N_("_Transfer..."), "<control>t",
	  N_("Transfer funds from one account to another"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_transfer) },
	{ "ActionsReconcileAction", NULL, N_("_Reconcile..."), "<control>r",
	  N_("Reconcile the selected account"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_reconcile) },
	{ "ActionsStockSplitAction", NULL, N_("Stock S_plit..."), NULL,
	  N_("Record a stock split or a stock merger"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_stock_split) },
	{ "ActionsLotsAction", NULL, N_("_Lot Viewer..."), NULL,
	  N_("Bring up the lot viewer/editor window"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_lots) },
	{ "RecordTransactionAction", GTK_STOCK_ADD, N_("_Enter Transaction"), NULL,
	  N_("Record the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_enter_transaction) },
	{ "CancelTransactionAction", GTK_STOCK_CANCEL, N_("_Cancel Transaction"), NULL,
	  N_("_Cancel the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_cancel_transaction) },
	{ "DeleteTransactionAction", GTK_STOCK_DELETE, N_("_Delete Transaction"), NULL,
	  N_("Delete the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_delete_transaction) },
	{ "BlankTransactionAction", GTK_STOCK_MISSING_IMAGE, N_("_Blank Transaction"), NULL,
	  N_("Move to the blank transaction at the bottom of the register"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_blank_transaction) },

	{ "DuplicateTransactionAction", GTK_STOCK_COPY, N_("D_uplicate Transaction"), NULL,
	  N_("Make a copy of the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_duplicate_transaction) },
	{ "RemoveTransactionSplitsAction", GTK_STOCK_CLEAR, N_("Remove Transaction Splits"), NULL,
	  N_("Remove all splits in the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_reinitialize_transaction) },
	{ "EditExchangeRateAction", GTK_STOCK_MISSING_IMAGE, N_("Edit Exchange Rate"), NULL,
	  N_("Exit the exchange rate for the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_exchange_rate) },
	{ "JumpTransactionAction", GTK_STOCK_JUMP_TO, N_("_Jump"), NULL,
	  N_("Jump to the corresponding transaction in the other account"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_jump) },
	{ "ScheduleTransactionAction", GTK_STOCK_COPY, N_("Schedule..."), NULL,
	  N_("Edit scheduled transactions"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_schedule) },

	/* Reports menu */
	{ "ReportsAccountReportAction", NULL, N_("_Account Report"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_account_report) },
	{ "ReportsTransactionReportAction", NULL, N_("_Transaction Report"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_transaction_report) },
};
static guint gnc_plugin_page_register_n_actions = G_N_ELEMENTS (gnc_plugin_page_register_actions);

static GtkToggleActionEntry toggle_entries[] = {
	{ "StatusReconciledAction", NULL, N_("_Reconciled"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_status_one), TRUE },
	{ "StatusClearedAction", NULL, N_("_Cleared"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_status_one), TRUE },
	{ "StatusVoidedAction", NULL, N_("_Voided"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_status_one), TRUE },
	{ "StatusFrozenAction", NULL, N_("_Frozen"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_status_one), TRUE },
	{ "StatusUnreconciledAction", NULL, N_("_Unreconciled"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_status_one), TRUE },

	{ "StyleDoubleLineAction", NULL, N_("_Double Line"), NULL, NULL,
	  G_CALLBACK (gnc_plugin_page_register_cmd_style_double_line), FALSE },

	{ "SplitTransactionAction", GTK_STOCK_MISSING_IMAGE, N_("S_plit Transaction"), NULL,
	  N_("Show all splits in the current transaction"),
	  G_CALLBACK (gnc_plugin_page_register_cmd_expand_transaction), FALSE },
};
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

static GtkRadioActionEntry radio_entries_1 [] =
{
	{ "SortStandardAction", NULL, N_("_Standard"), NULL, NULL, BY_STANDARD },
	{ "SortDateAction", NULL, N_("_Date"), NULL, NULL, BY_DATE },
	{ "SortDateEntryAction", NULL, N_("Dateof _Entry"), NULL, NULL, BY_DATE_ENTERED },
	{ "SortDateStatementAction", NULL, N_("_Statement Date"), NULL, NULL, BY_DATE_RECONCILED },
	{ "SortNumberAction", NULL, N_("_Number"), NULL, NULL, BY_NUM },
	{ "SortAmountAction", NULL, N_("_Amount"), NULL, NULL, BY_AMOUNT },
	{ "SortMemoAction", NULL, N_("_Memo"), NULL, NULL, BY_MEMO },
	{ "SortDescriptionAction", NULL, N_("Descri_ption"), NULL, NULL, BY_DESC },
	{ "SortActionAction", NULL, N_("A_ction"), NULL, NULL, BY_ACTION },
	{ "SortNotesAction", NULL, N_("N_otes"), NULL, NULL, BY_NOTES },
};
static guint n_radio_entries_1 = G_N_ELEMENTS (radio_entries_1);

static GtkRadioActionEntry radio_entries_2 [] =
{
	{ "StyleBasicAction", NULL, N_("_Basic Ledger"), NULL, NULL, REG_STYLE_LEDGER },
	{ "StyleAutoSplitAction", NULL, N_("_Auto-Split Ledger"), NULL, NULL, REG_STYLE_AUTO_LEDGER },
	{ "StyleJournalAction", NULL, N_("_Transaction Journal"), NULL, NULL, REG_STYLE_JOURNAL }
};
static guint n_radio_entries_2 = G_N_ELEMENTS (radio_entries_2);

static const gchar *actions_requiring_account[] = {
	"ActionsReconcileAction",
	"ActionsLotsAction",
	NULL
};

/* DRH - Suggest short_labels be added to libegg */
static action_short_labels short_labels[] = {
  { "ActionsTransferAction", 	  N_("Transfer") },
  { "RecordTransactionAction", 	  N_("Enter") },
  { "CancelTransactionAction", 	  N_("Cancel") },
  { "DeleteTransactionAction", 	  N_("Delete") },
  { "DuplicateTransactionAction", N_("Duplicate") },
  { "SplitTransactionAction",     N_("Split") },
  { "ScheduleTransactionAction",  N_("Schedule") },
  { "BlankTransactionAction",     N_("Blank") },
  { NULL, NULL },
};


struct {
  const char *action_name;
  int value;
} status_actions[] = {
  { "StatusReconciledAction", 	CLEARED_RECONCILED },
  { "StatusClearedAction",    	CLEARED_CLEARED },
  { "StatusVoidedAction",     	CLEARED_VOIDED },
  { "StatusFrozenAction",     	CLEARED_FROZEN },
  { "StatusUnreconciledAction", CLEARED_NO },
  { NULL, 0 },
};
#define CLEARED_VALUE "cleared_value"


/************************************************************/
/*                      Data Structures                     */
/************************************************************/

struct GncPluginPageRegisterPrivate
{
	GtkActionGroup *action_group;
	guint merge_id;
	GtkUIManager *ui_merge;

	GNCLedgerDisplay *ledger;
	GNCSplitReg *gsr;

	GtkWidget *widget;

	char *ui_description;

	const char *lines_opt_page;
	const char *lines_opt_name;
	gint lines_default;
	gint disallowCaps;

	cleared_match_t cleared_match;
};

typedef struct _RegDateWindow RegDateWindow;
struct _RegDateWindow
{
  GtkWidget       *dialog;

  GtkToggleButton *start_date_radio;
  GtkWidget       *start_date;

  GtkToggleButton *end_date_radio;
  GtkWidget 	  *end_date;
  GtkWidget 	  *today_button;

  GtkWidget 	  *apply_button;
};

static GObjectClass *parent_class = NULL;

/************************************************************/
/*                      Implementation                      */
/************************************************************/

GType
gnc_plugin_page_register_get_type (void)
{
	static GType gnc_plugin_page_register_type = 0;

	if (gnc_plugin_page_register_type == 0) {
		static const GTypeInfo our_info = {
			sizeof (GncPluginPageRegisterClass),
			NULL,
			NULL,
			(GClassInitFunc) gnc_plugin_page_register_class_init,
			NULL,
			NULL,
			sizeof (GncPluginPageRegister),
			0,
			(GInstanceInitFunc) gnc_plugin_page_register_init
		};
		
		gnc_plugin_page_register_type = g_type_register_static (GNC_TYPE_PLUGIN_PAGE,
									"GncPluginPageRegister",
									&our_info, 0);
	}

	return gnc_plugin_page_register_type;
}

static GncPluginPage *
gnc_plugin_page_register_new_common (GNCLedgerDisplay *ledger)
{
	GncPluginPageRegister *register_page;
	GncPluginPage *plugin_page;
	GNCSplitReg *gsr;
	GList *item, *book_list;
	QofQuery *q;

	/* Is there an existing page? */
	gsr = gnc_ledger_display_get_user_data (ledger);
	if (gsr) {
	  for (item = active_pages; item; item = g_list_next(item)) {
	    register_page = (GncPluginPageRegister *)item->data;
	    if (register_page->priv->gsr == gsr)
	      return GNC_PLUGIN_PAGE(register_page);
	  }
	}

	register_page = g_object_new (GNC_TYPE_PLUGIN_PAGE_REGISTER, NULL);
	register_page->priv->ledger = ledger;

	plugin_page = GNC_PLUGIN_PAGE(register_page);
	plugin_page->title = gnc_plugin_page_register_get_tab_name(plugin_page);
	plugin_page->tab_name = gnc_plugin_page_register_get_tab_name(plugin_page);

	q = gnc_ledger_display_get_query (ledger);
	book_list = qof_query_get_books (q);
	for (item = book_list; item; item = g_list_next(item))
	  gnc_plugin_page_add_book (plugin_page, (QofBook *)item->data);
	// Do not free the list. It is owned by the query.
	
	g_object_set_data(G_OBJECT(plugin_page),
			  GNC_PLUGIN_HIDE_MENU_ADDITIONS_NAME,
			  GINT_TO_POINTER(1));

	return plugin_page;
}

GncPluginPage *
gnc_plugin_page_register_new (Account *account, gboolean subaccounts)
{
	GNCLedgerDisplay *ledger;

	if (subaccounts)
	  ledger = gnc_ledger_display_subaccounts (account);
	else
	  ledger = gnc_ledger_display_simple (account);

	return gnc_plugin_page_register_new_common(ledger);
}

GncPluginPage *
gnc_plugin_page_register_new_gl (void)
{
	GNCLedgerDisplay *ledger;

	ledger = gnc_ledger_display_gl ();
	return gnc_plugin_page_register_new_common(ledger);
}

GncPluginPage *
gnc_plugin_page_register_new_ledger (GNCLedgerDisplay *ledger)
{
	return gnc_plugin_page_register_new_common(ledger);
}

#if DEBUG_REFERENCE_COUNTING
static void
dump_model (GncPluginPageRegister *page, gpointer dummy)
{
    g_warning("GncPluginPageRegister %p still exists.", page);
}

static gint
gnc_plugin_page_register_report_references (void)
{
  g_list_foreach(active_pages, (GFunc)dump_model, NULL);
  return 0;
}
#endif

static void
gnc_plugin_page_register_class_init (GncPluginPageRegisterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GncPluginPageClass *gnc_plugin_class = GNC_PLUGIN_PAGE_CLASS(klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = gnc_plugin_page_register_finalize;

	gnc_plugin_class->tab_icon        = GNC_STOCK_ACCOUNT;
	gnc_plugin_class->plugin_name     = GNC_PLUGIN_PAGE_REGISTER_NAME;
	gnc_plugin_class->create_widget   = gnc_plugin_page_register_create_widget;
	gnc_plugin_class->destroy_widget  = gnc_plugin_page_register_destroy_widget;
	gnc_plugin_class->merge_actions   = gnc_plugin_page_register_merge_actions;
	gnc_plugin_class->unmerge_actions = gnc_plugin_page_register_unmerge_actions;

#if DEBUG_REFERENCE_COUNTING
	gtk_quit_add (0,
		      (GtkFunction)gnc_plugin_page_register_report_references,
		      NULL);
#endif
}

static void
gnc_plugin_page_register_init_values (GtkActionGroup *action_group)
{
  GtkAction *action;
  gint i;

  for (i = 0; status_actions[i].action_name; i++) {
    /* Add a couple of short labels for the toolbar */
    action = gtk_action_group_get_action (action_group,
					  status_actions[i].action_name);
    g_object_set_data (G_OBJECT(action), CLEARED_VALUE,
		       GINT_TO_POINTER(status_actions[i].value));
  }
}

static void
gnc_plugin_page_register_init (GncPluginPageRegister *plugin_page)
{
	GncPluginPageRegisterPrivate *priv;
	GncPluginPage *parent;
	GtkActionGroup *action_group;

	priv = g_new0 (GncPluginPageRegisterPrivate, 1);
	plugin_page->priv = priv;

	/* Init parent declared variables */
	parent = GNC_PLUGIN_PAGE(plugin_page);
	parent->title       = g_strdup(_("General Ledger"));
	parent->tab_name    = g_strdup(_("General Ledger"));
	parent->uri         = g_strdup("default:");

	/* Create menu and toolbar information */
	action_group = gtk_action_group_new ("GncPluginPageRegisterActions");
	priv->action_group = action_group;
	gtk_action_group_add_actions (action_group, gnc_plugin_page_register_actions,
				      gnc_plugin_page_register_n_actions, plugin_page);
	gtk_action_group_add_toggle_actions (action_group,
					     toggle_entries, n_toggle_entries,
					     plugin_page);
	gtk_action_group_add_radio_actions (action_group,
					    radio_entries_1, n_radio_entries_1,
					    REG_STYLE_LEDGER,
					    G_CALLBACK(gnc_plugin_page_register_cmd_sort_changed),
					    plugin_page);
	gtk_action_group_add_radio_actions (action_group,
					    radio_entries_2, n_radio_entries_2,
					    REG_STYLE_LEDGER,
					    G_CALLBACK(gnc_plugin_page_register_cmd_style_changed),
					    plugin_page);

	gnc_plugin_init_short_names (action_group, short_labels);
	gnc_plugin_page_register_init_values (action_group);

	priv->ui_description = g_strdup("gnc-plugin-page-register-ui.xml");

	priv->lines_opt_page = DEFAULT_LINES_OPTION_PAGE;
	priv->lines_opt_name = DEFAULT_LINES_OPTION_NAME;
	priv->lines_default  = DEFAULT_LINES_AMOUNT;
	priv->disallowCaps = 0;
	priv->cleared_match = CLEARED_ALL;

	active_pages = g_list_append (active_pages, plugin_page);
}

static void
gnc_plugin_page_register_finalize (GObject *object)
{
	GncPluginPageRegister *page;

	ENTER("object %p", object);
	page = GNC_PLUGIN_PAGE_REGISTER (object);

	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (page));
	g_return_if_fail (page->priv != NULL);

	active_pages = g_list_remove (active_pages, page);

	g_free (page->priv->ui_description);
	g_free (page->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
	LEAVE(" ");
}


Account *
gnc_plugin_page_register_get_account (GncPluginPageRegister *page)
{ 
	GNCLedgerDisplayType ledger_type;
	Account *leader;

	ledger_type = gnc_ledger_display_type (page->priv->ledger);
	leader = gnc_ledger_display_leader (page->priv->ledger);

	if ((ledger_type == LD_SINGLE) || (ledger_type == LD_SUBACCOUNT))
	  return leader;
	return NULL;
}


static void
gnc_plugin_page_register_update_menus (GncPluginPageRegister *page)
{ 
	GncPluginPageRegisterPrivate *priv ;
	Account *account;
	SplitRegister *sr;
	GtkAction *action;

	priv = page->priv;
	account = gnc_plugin_page_register_get_account (page);
	gnc_plugin_update_actions(priv->action_group, actions_requiring_account,
				  "sensitive", account != NULL);

	sr = gnc_ledger_display_get_split_register(priv->ledger);
	action = gtk_action_group_get_action (priv->action_group,
					      "StyleDoubleLineAction");
	g_signal_handlers_block_by_func(action, gnc_plugin_page_register_cmd_style_double_line, page);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(action), sr->use_double_line);
	g_signal_handlers_unblock_by_func(action, gnc_plugin_page_register_cmd_style_double_line, page);
}


/* Virtual Functions */

static GtkWidget *
gnc_plugin_page_register_create_widget (GncPluginPage *plugin_page)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;
	guint numRows;
	GtkWidget *gsr;
	SplitRegister *sr;

	ENTER("page %p", plugin_page);
	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;
	if (priv->widget != NULL)
		return priv->widget;

	priv->widget = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (priv->widget);
	
	numRows = (guint) gnc_lookup_number_option (priv->lines_opt_page,
						    priv->lines_opt_name,
						    priv->lines_default);

	gsr = gnc_split_reg_new(priv->ledger, NULL, numRows,
				0, priv->disallowCaps);
	priv->gsr = (GNCSplitReg *)gsr;
	gtk_widget_show (gsr);
	gtk_box_pack_start (GTK_BOX (priv->widget), gsr, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (gsr), "help-changed",
			  G_CALLBACK ( gnc_plugin_page_help_changed_cb ),
			  page );
	g_signal_connect (G_OBJECT (gsr), "button-press-event",
			  G_CALLBACK (gnc_plugin_page_register_button_press_cb), page);

	sr = gnc_ledger_display_get_split_register(priv->ledger);
	gnc_split_register_config(sr, sr->type, sr->style, sr->use_double_line);
	gnc_ledger_display_refresh(priv->ledger);

	gnc_plugin_page_register_update_menus (page);

	/* DRH - Probably lots of other stuff from regWindowLedger should end up here. */
	return priv->widget;
}

static void
gnc_plugin_page_register_destroy_widget (GncPluginPage *plugin_page)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	ENTER("page %p", plugin_page);
	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;

	if (priv->widget == NULL)
		return;

	gtk_widget_hide(priv->widget);
	gnc_ledger_display_close (priv->ledger);
	priv->ledger = NULL;
}

static void
gnc_plugin_page_register_merge_actions (GncPluginPage *plugin_page,
					GtkUIManager *ui_merge)
{
	GncPluginPageRegister *register_page;
	GncPluginPageRegisterPrivate *priv;
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page));

	register_page = GNC_PLUGIN_PAGE_REGISTER(plugin_page);
	priv = register_page->priv;

	priv->ui_merge = ui_merge;
	priv->merge_id = gnc_plugin_add_actions (priv->ui_merge,
						 priv->action_group,
						 priv->ui_description);
}
	
static void
gnc_plugin_page_register_unmerge_actions (GncPluginPage *plugin_page,
					      GtkUIManager *ui_merge)
{
	GncPluginPageRegister *plugin_page_register = GNC_PLUGIN_PAGE_REGISTER(plugin_page);
	
	g_return_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page_register));
	g_return_if_fail (plugin_page_register->priv->merge_id != 0);
	g_return_if_fail (plugin_page_register->priv->action_group != NULL);

	gtk_ui_manager_remove_ui (ui_merge, plugin_page_register->priv->merge_id);
	gtk_ui_manager_remove_action_group (ui_merge, plugin_page_register->priv->action_group);

	plugin_page_register->priv->ui_merge = NULL;
}

static gchar *
gnc_plugin_page_register_get_tab_name (GncPluginPage *plugin_page)
{
	GNCLedgerDisplayType ledger_type;
  	GNCLedgerDisplay *ld;
	SplitRegister *reg;
	Account *leader;

	g_return_val_if_fail (GNC_IS_PLUGIN_PAGE_REGISTER (plugin_page), _("unknown"));

	ld = GNC_PLUGIN_PAGE_REGISTER (plugin_page)->priv->ledger;
	reg = gnc_ledger_display_get_split_register (ld);
	ledger_type = gnc_ledger_display_type (ld);
	leader = gnc_ledger_display_leader (ld);

	switch (ledger_type) {
	 case LD_SINGLE:
	  return g_strdup(xaccAccountGetName (leader));

	 case LD_SUBACCOUNT:
	  return g_strdup_printf("%s+", xaccAccountGetName (leader));

	 case LD_GL:
	  switch (reg->type) {
	   case GENERAL_LEDGER:
	   case INCOME_LEDGER:
	    return g_strdup(_("General Ledger"));
	   case PORTFOLIO_LEDGER:
	    return g_strdup(_("Portfolio"));
	   case SEARCH_LEDGER:
	    return g_strdup(_("Search Results"));
	   default:
	    break;
	  }

	 default:
	  break;
	}

	return g_strdup(_("unknown"));
}

/* Callbacks */
static gboolean
gnc_plugin_page_register_button_press_cb (GtkWidget *widget,
					  GdkEventButton *event,
					  GncPluginPageRegister *page)
{
	GtkWidget *menu;

	if (event->button == 3 && page->priv->ui_merge != NULL) {
		/* Maybe show a different popup menu if no account is selected. */
		menu = gtk_ui_manager_get_widget (page->priv->ui_merge, "/RegisterPopup");
		if (menu)
		  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
				  event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

/************************************************************/
/*                     Set Date Window                      */
/************************************************************/

void
gnc_plugin_page_register_start_toggle_cb(GtkToggleButton *toggle, gpointer data)
{
  RegDateWindow *regDateData = data;
  gboolean user_date;

  g_return_if_fail(regDateData != NULL);

  user_date = gtk_toggle_button_get_active(regDateData->start_date_radio);
  gtk_widget_set_sensitive(regDateData->start_date, user_date);
  gtk_widget_set_sensitive(regDateData->apply_button, TRUE);
}

void
gnc_plugin_page_register_end_toggle_cb(GtkToggleButton *toggle, gpointer data)
{
  RegDateWindow *regDateData = data;
  gboolean user_date;

  g_return_if_fail(regDateData != NULL);

  user_date = gtk_toggle_button_get_active(regDateData->end_date_radio);
  gtk_widget_set_sensitive(regDateData->end_date, user_date);
  gtk_widget_set_sensitive(regDateData->today_button, user_date);
  gtk_widget_set_sensitive(regDateData->apply_button, TRUE);
}

void
gnc_plugin_page_register_today_cb(GtkButton *buttontoggle, gpointer data)
{
  RegDateWindow *regDateData = data;

  g_return_if_fail(regDateData != NULL);

  gnc_date_edit_set_time(GNC_DATE_EDIT(regDateData->end_date), time(NULL));
  gtk_widget_set_sensitive(regDateData->apply_button, TRUE);
}

static void
gnc_plugin_page_register_changed_cb (GtkWidget *unused, gpointer data)
{
  RegDateWindow *regDateData = data;

  g_return_if_fail(regDateData != NULL);

  gtk_widget_set_sensitive(regDateData->apply_button, TRUE);
}

static void
connect_gnc_date_entry (GtkWidget *date, gpointer data)
{
    GtkWidget *calendar;
    GtkWidget *entry;

    g_signal_connect (G_OBJECT (date), "date-changed",
                      G_CALLBACK (gnc_plugin_page_register_changed_cb), data);

    calendar = GNC_DATE_EDIT(date)->calendar;
    g_signal_connect (G_OBJECT (calendar), "day_selected_double_click",
		      G_CALLBACK (gnc_plugin_page_register_changed_cb), data);

    entry = GNC_DATE_EDIT(date)->date_entry;
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    g_signal_connect (G_OBJECT (entry), "activate",
		      G_CALLBACK (gnc_plugin_page_register_changed_cb), data);
    g_signal_connect (G_OBJECT (entry), "changed",
		      G_CALLBACK (gnc_plugin_page_register_changed_cb), data);
}

static RegDateWindow *
build_date_range_dialog (GncPluginPageRegister *page)
{
  RegDateWindow *regDateData;
  GtkWidget *window;
  GladeXML *xml;
  GtkWidget *date, *hbox, *button;
  Query *query;
  time_t start_time = 0, end_time = 0, time_val;

  regDateData = g_new0(RegDateWindow, 1);
  query = gnc_ledger_display_get_query (page->priv->ledger);

  xml = gnc_glade_xml_new ("register.glade", "Date Range");
  regDateData->dialog = glade_xml_get_widget (xml, "Date Range");

  window = GNC_PLUGIN_PAGE (page)->window;
  gtk_window_set_transient_for (GTK_WINDOW (regDateData->dialog),
				GTK_WINDOW (window));

  button = glade_xml_get_widget(xml, "start_date_radio");
  regDateData->start_date_radio = GTK_TOGGLE_BUTTON(button);
  button = glade_xml_get_widget(xml, "end_date_radio");
  regDateData->end_date_radio = GTK_TOGGLE_BUTTON(button);
  regDateData->today_button = glade_xml_get_widget(xml, "today");
  regDateData->apply_button = glade_xml_get_widget(xml, "apply_button");
  regDateData->start_date = gnc_date_edit_new(time(NULL), FALSE, FALSE);
  regDateData->end_date = gnc_date_edit_new(time(NULL), FALSE, FALSE);

  /* Is this register date limited? */
  xaccQueryGetDateMatchTT(query, &start_time, &end_time);

  /* Starting Date */
  hbox = glade_xml_get_widget(xml, "start_date_entry");
  date = regDateData->start_date;
  gtk_widget_show(date);
  gtk_box_pack_start(GTK_BOX(hbox), date, FALSE, FALSE, 0);

  if (start_time) {
    gnc_date_edit_set_time(GNC_DATE_EDIT(date), start_time);
    gtk_toggle_button_set_active(regDateData->start_date_radio, TRUE);
  } else {
    time_val = xaccQueryGetEarliestDateFound (query); 
    gnc_date_edit_set_time(GNC_DATE_EDIT(date), time_val);
    gtk_widget_set_sensitive(date, FALSE);
  }
  connect_gnc_date_entry (date, regDateData);

  /* Ending Date */
  hbox = glade_xml_get_widget(xml, "end_date_entry");
  date = regDateData->end_date;
  gtk_widget_show(date);
  gtk_box_pack_start(GTK_BOX(hbox), date, FALSE, FALSE, 0);

  if (end_time) {
    gnc_date_edit_set_time(GNC_DATE_EDIT(date), end_time);
    gtk_toggle_button_set_active(regDateData->end_date_radio, TRUE);
  } else {
    time_val = xaccQueryGetLatestDateFound (query);
    gnc_date_edit_set_time(GNC_DATE_EDIT(date), time_val);
    gtk_widget_set_sensitive(date, FALSE);
    gtk_widget_set_sensitive(regDateData->today_button, TRUE);
  }
  connect_gnc_date_entry (date, regDateData);

  /*
   * Wire up the callbacks last, so the above widget manipulation
   * doesn't generate spurious callbacks.
   */
  glade_xml_signal_autoconnect_full(xml, gnc_glade_autoconnect_full_func,
				    regDateData);
  return regDateData;
}

static void
set_date_range (GncPluginPageRegisterPrivate *priv,
		RegDateWindow *regDateData)
{
  GSList *param_list;
  gboolean start_set, end_set;
  Query *query;
  time_t start_time, end_time;

  ENTER(" ");
  if (!priv->ledger) {
    LEAVE("no ledger");
    return;
  }

  query = gnc_ledger_display_get_query( priv->ledger );
  if (!query) {
    LEAVE("no query");
    return;
  }

  /* Delete any existing old date spec. */
  param_list = gncQueryBuildParamList(SPLIT_TRANS, TRANS_DATE_POSTED, NULL);
  if (param_list) {
    gncQueryPurgeTerms (query, param_list);
    g_slist_free(param_list);
  }

  start_set = gtk_toggle_button_get_active(regDateData->start_date_radio);
  end_set = gtk_toggle_button_get_active(regDateData->end_date_radio);
  if (!start_set && !end_set) {
    LEAVE(" ");
    return;
  }
    
  /* Build a new spec */
  start_time= gnc_date_edit_get_date(GNC_DATE_EDIT(regDateData->start_date));
  start_time = gnc_timet_get_day_start(start_time);

  end_time = gnc_date_edit_get_date(GNC_DATE_EDIT(regDateData->end_date));
  end_time = gnc_timet_get_day_end(end_time);

  xaccQueryAddDateMatchTT(query,
			  start_set, start_time,
			  end_set, end_time,
			  QUERY_AND);
  LEAVE(" ");
}

/************************************************************/
/*                  Report Helper Functions                 */
/************************************************************/

static char *
gnc_reg_get_name (GNCLedgerDisplay *ledger, gboolean for_window)
{
  Account *leader;
  SplitRegister *reg;
  gchar *account_name;
  gchar *reg_name;
  gchar *name;
  GNCLedgerDisplayType ledger_type;

  if (ledger == NULL)
    return NULL;

  reg = gnc_ledger_display_get_split_register (ledger);
  ledger_type = gnc_ledger_display_type (ledger);

  switch (reg->type)
  {
    case GENERAL_LEDGER:
    case INCOME_LEDGER:
      if (for_window)
        reg_name = _("General Ledger");
      else
        reg_name = _("General Ledger Report");
      break;
    case PORTFOLIO_LEDGER:
      if (for_window)
        reg_name = _("Portfolio");
      else
        reg_name = _("Portfolio Report");
      break;
    case SEARCH_LEDGER:
      if (for_window)
        reg_name = _("Search Results");
      else
        reg_name = _("Search Results Report");
      break;
    default:
      if (for_window)
        reg_name = _("Register");
      else
        reg_name = _("Register Report");
      break;
  }

  leader = gnc_ledger_display_leader (ledger);

  if ((leader != NULL) && (ledger_type != LD_GL))
  {
    account_name = xaccAccountGetFullName (leader,
                                           gnc_get_account_separator ());

    if (ledger_type == LD_SINGLE)
    {
      name = g_strconcat (account_name, " - ", reg_name, NULL);
    }
    else 
    {
      name = g_strconcat (account_name, " ", _("and subaccounts"), " - ", reg_name, NULL);
    }
    g_free(account_name);
  }
  else
    name = g_strdup (reg_name);

  return name;
}

static int
report_helper (GNCLedgerDisplay *ledger, Split *split, Query *query)
{
  SplitRegister *reg = gnc_ledger_display_get_split_register (ledger);
  Account *account;
  char *str;
  SCM qtype;
  SCM args;
  SCM func;
  SCM arg;

  args = SCM_EOL;

  func = scm_c_eval_string ("gnc:register-report-create");
  g_return_val_if_fail (SCM_PROCEDUREP (func), -1);

  arg = scm_makfrom0str (gnc_split_register_get_credit_string (reg));
  args = scm_cons (arg, args);

  arg = scm_makfrom0str (gnc_split_register_get_debit_string (reg));
  args = scm_cons (arg, args);

  str = gnc_reg_get_name (ledger, FALSE);
  arg = scm_makfrom0str (str);
  args = scm_cons (arg, args);
  g_free (str);

  arg = SCM_BOOL (reg->use_double_line);
  args = scm_cons (arg, args);

  arg = SCM_BOOL (reg->style == REG_STYLE_JOURNAL);
  args = scm_cons (arg, args);

  if (!query)
  {
    query = gnc_ledger_display_get_query (ledger);
    g_return_val_if_fail (query != NULL, -1);
  }

  qtype = scm_c_eval_string("<gnc:Query*>");
  g_return_val_if_fail (qtype != SCM_UNDEFINED, -1);

  arg = gw_wcp_assimilate_ptr (query, qtype);
  args = scm_cons (arg, args);
  g_return_val_if_fail (arg != SCM_UNDEFINED, -1);


  if (split)
  {
    qtype = scm_c_eval_string("<gnc:Split*>");
    g_return_val_if_fail (qtype != SCM_UNDEFINED, -1);
    arg = gw_wcp_assimilate_ptr (split, qtype);
  }
  else
  {
    arg = SCM_BOOL_F;
  }
  args = scm_cons (arg, args);
  g_return_val_if_fail (arg != SCM_UNDEFINED, -1);


  qtype = scm_c_eval_string("<gnc:Account*>");
  g_return_val_if_fail (qtype != SCM_UNDEFINED, -1);

  account = gnc_ledger_display_leader (ledger);
  arg = gw_wcp_assimilate_ptr (account, qtype);
  args = scm_cons (arg, args);
  g_return_val_if_fail (arg != SCM_UNDEFINED, -1);


  /* Apply the function to the args */
  arg = scm_apply (func, args, SCM_EOL);
  g_return_val_if_fail (SCM_EXACTP (arg), -1);

  return scm_num2int (arg, SCM_ARG1, __FUNCTION__);
}

/************************************************************/
/*                     Command callbacks                    */
/************************************************************/

static void
gnc_plugin_page_register_cmd_print_check (GtkAction *action,
					  GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister * reg;
  Split         * split;
  Transaction   * trans;
  const char    * payee;
  const char    * memo;
  gnc_numeric   amount;
  time_t        date;


  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);
  split    = gnc_split_register_get_current_split(reg);
  trans    = xaccSplitGetParent(split);

  if(split && trans)
  {
    payee  = xaccTransGetDescription(trans);
    memo   = xaccTransGetNotes(trans);
    if (memo == NULL)
      memo = "";
    amount = xaccSplitGetAmount(split);
    amount = gnc_numeric_abs (amount);
    date   = xaccTransGetDate(trans);

    gnc_ui_print_check_dialog_create(plugin_page, payee, amount, date, memo);
  }
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_date_set_range (GtkAction *action,
					     GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  RegDateWindow *date_window;
  gint response;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  date_window = build_date_range_dialog (plugin_page);
  response = gtk_dialog_run (GTK_DIALOG(date_window->dialog));
  switch (response) {
   case GTK_RESPONSE_APPLY:
    set_date_range (plugin_page->priv, date_window);
    gnc_ledger_display_refresh (plugin_page->priv->ledger);
    break;
   default:
    break;
  }
  gtk_widget_destroy(date_window->dialog);
  g_free (date_window);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_date_show_all (GtkAction *action,
					    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  GSList *param_list;
  Query *query;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  priv = plugin_page->priv;
  if (!priv->ledger) {
    LEAVE("no ledger");
    return;
  }

  query = gnc_ledger_display_get_query( priv->ledger );
  if (!query) {
    LEAVE("no query");
    return;
  }

  /* Delete any existing date spec. */
  param_list = gncQueryBuildParamList(SPLIT_TRANS, TRANS_DATE_POSTED, NULL);
  if (param_list) {
    gncQueryPurgeTerms (query, param_list);
    g_slist_free(param_list);
  }
  gnc_ledger_display_refresh (priv->ledger);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_status_all (GtkAction *action,
					 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  GSList *param_list = NULL;
  GtkAction *other_action;
  Query *query;
  gint i;

  ENTER("(toggle action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  query = gnc_ledger_display_get_query( priv->ledger );
  if (!query) {
    LEAVE("no query found");
    return;
  }

  /* Turn off all the other check menu items */
  for (i = 0; status_actions[i].action_name; i++) {
    other_action = gtk_action_group_get_action (priv->action_group,
						status_actions[i].action_name);
    g_signal_handlers_block_by_func(other_action, gnc_plugin_page_register_cmd_status_one, plugin_page);
    gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(other_action), TRUE);
    g_signal_handlers_unblock_by_func(other_action, gnc_plugin_page_register_cmd_status_one, plugin_page);
  }

  /* Remove the old status match */
  param_list = gncQueryBuildParamList (SPLIT_RECONCILE, NULL);
  if (param_list) {
    gncQueryPurgeTerms (query, param_list);
    g_slist_free(param_list);
  }
  gnc_ledger_display_refresh (priv->ledger);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_status_one (GtkToggleAction *action,
					 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  GSList *param_list = NULL;
  Query *query;
  gint value;

  ENTER("(toggle action %p (%s), plugin_page %p)",
	action, action ? gtk_action_get_name(GTK_ACTION(action)) : "<no name>",
	plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  query = gnc_ledger_display_get_query( priv->ledger );
  if (!query) {
    LEAVE("no query found");
    return;
  }

  /* Compute the new match status */
  value = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(action), CLEARED_VALUE));
  if (gtk_toggle_action_get_active (action))
    priv->cleared_match |= value;
  else
    priv->cleared_match &= ~value;

  /* Remove the old status match */
  param_list = gncQueryBuildParamList (SPLIT_RECONCILE, NULL);
  if (param_list) {
    gncQueryPurgeTerms (query, param_list);
    g_slist_free(param_list);
  }

  /* Install the new status match */
  if (priv->cleared_match != CLEARED_ALL)
    xaccQueryAddClearedMatch(query, priv->cleared_match, QUERY_AND);

  gnc_ledger_display_refresh (priv->ledger);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_sort_changed (GtkAction *action,
					   GtkRadioAction *current,
					   GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SortType value;

  ENTER("(action %p, radio action %p, plugin_page %p)",
	action, current, plugin_page);

  g_return_if_fail(GTK_IS_ACTION(action));
  g_return_if_fail(GTK_IS_RADIO_ACTION(current));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  value = gtk_radio_action_get_current_value(current);
  gnc_split_reg_set_sort_type(priv->gsr, value);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_style_changed (GtkAction *action,
					    GtkRadioAction *current,
					    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegisterStyle value;

  ENTER("(action %p, radio action %p, plugin_page %p)",
	action, current, plugin_page);

  g_return_if_fail(GTK_IS_ACTION(action));
  g_return_if_fail(GTK_IS_RADIO_ACTION(current));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  value = gtk_radio_action_get_current_value(current);
  gnc_split_reg_change_style(priv->gsr, value);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_style_double_line (GtkToggleAction *action,
						GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;
  gboolean use_double_line;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GTK_IS_ACTION(action));
  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  use_double_line =  gtk_toggle_action_get_active (action);
  if (use_double_line != reg->use_double_line) {
    gnc_split_register_config(reg, reg->type, reg->style, use_double_line);
    gnc_ledger_display_refresh(priv->ledger);
  }
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_transfer (GtkAction *action,
				       GncPluginPageRegister *page)
{
  Account *account;
  GtkWidget *window;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);
  window = GNC_PLUGIN_PAGE (page)->window;
  gnc_xfer_dialog (window, account);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_reconcile (GtkAction *action,
					GncPluginPageRegister *page)
{
  Account *account;
  GtkWidget *window;
  RecnWindow * recnData;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);

  window = GNC_PLUGIN_PAGE (page)->window;
  recnData = recnWindow (window, account);
  gnc_ui_reconcile_window_raise (recnData);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_stock_split (GtkAction *action,
					  GncPluginPageRegister *page)
{
  Account *account;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);
  gnc_stock_split_dialog (NULL, account);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_lots (GtkAction *action,
				   GncPluginPageRegister *page)
{
  Account *account;

  ENTER("(action %p, plugin_page %p)", action, page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(page));

  account = gnc_plugin_page_register_get_account (page);
  gnc_lot_viewer_dialog (account);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_enter_transaction (GtkAction *action,
						GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gnc_split_reg_enter(priv->gsr, FALSE);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_cancel_transaction (GtkAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gnc_split_register_cancel_cursor_trans_changes
    (gnc_ledger_display_get_split_register(priv->ledger));
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_delete_transaction (GtkAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gsr_default_delete_handler(priv->gsr, NULL);
  LEAVE(" ");

}

static void
gnc_plugin_page_register_cmd_blank_transaction (GtkAction *action,
						GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  if (gnc_split_register_save (reg, TRUE))
    gnc_split_register_redraw (reg);

  gnc_split_reg_jump_to_blank (priv->gsr);
}

static void
gnc_plugin_page_register_cmd_duplicate_transaction (GtkAction *action,
						    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gnc_split_register_duplicate_current
    (gnc_ledger_display_get_split_register(priv->ledger));
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_reinitialize_transaction (GtkAction *action,
						       GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gsr_default_reinit_handler(priv->gsr, NULL);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_expand_transaction (GtkToggleAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;
  gboolean expand;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);
  expand = gtk_toggle_action_get_active (action);
  gnc_split_register_expand_current_trans (reg, expand);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_exchange_rate (GtkAction *action,
					    GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  /* XXX Ignore the return value -- we don't care if this succeeds */
  (void)gnc_split_register_handle_exchange (reg, TRUE);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_jump (GtkAction *action,
				   GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  GncPluginPage *new_page;
  GncPluginPageRegister *new_reg_page;
  GtkWidget *window;
  GNCSplitReg *gsr;
  SplitRegister *reg;
  Account *account;
  Account *leader;
  Split *split;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  window = GNC_PLUGIN_PAGE (plugin_page)->window;
  if (window == NULL) {
    LEAVE("no window");
    return;
  }

  reg = gnc_ledger_display_get_split_register (priv->ledger);
  split = gnc_split_register_get_current_split (reg);
  if (split == NULL) {
    LEAVE("no split (1)");
    return;
  }

  account = xaccSplitGetAccount (split);
  if (account == NULL) {
    LEAVE("no account");
    return;
  }

  leader = gnc_ledger_display_leader (priv->ledger);
  if (account == leader)
  {
    split = xaccSplitGetOtherSplit (split);
    if (split == NULL) {
      LEAVE("no split (2)");
      return;
    }

    account = xaccSplitGetAccount (split);
    if (account == NULL) {
      LEAVE("no account (2)");
      return;
    }

    if (account == leader) {
      LEAVE("register open for account");
      return;
    }
  }

  new_page = gnc_plugin_page_register_new (account, FALSE);
  if (new_page == NULL) {
    LEAVE("couldn't create new page");
    return;
  }
  new_reg_page = GNC_PLUGIN_PAGE_REGISTER(new_page);

  /* DRH - This will be a problem when someone attempts to use the
     'Jump' command from the scheduled transaction window. */
  gnc_main_window_open_page (GNC_MAIN_WINDOW(window), new_page);
  gsr = gnc_plugin_page_register_get_gsr (new_page);
  gnc_split_reg_jump_to_split(gsr, split);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_schedule (GtkAction *action,
				       GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  gsr_default_schedule_handler(priv->gsr, NULL);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_account_report (GtkAction *action,
					     GncPluginPageRegister *plugin_page)
{
  GncMainWindow *window;
  int id;

  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  window = GNC_MAIN_WINDOW(GNC_PLUGIN_PAGE(plugin_page)->window);
  id = report_helper (plugin_page->priv->ledger, NULL, NULL);
  if (id >= 0)
    gnc_main_window_open_report(id, window);
  LEAVE(" ");
}

static void
gnc_plugin_page_register_cmd_transaction_report (GtkAction *action,
						 GncPluginPageRegister *plugin_page)
{
  GncPluginPageRegisterPrivate *priv;
  SplitRegister *reg;
  Split *split;
  Query *query;
  int id;


  ENTER("(action %p, plugin_page %p)", action, plugin_page);

  g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

  priv = plugin_page->priv;
  reg = gnc_ledger_display_get_split_register (priv->ledger);

  split = gnc_split_register_get_current_split (reg);
  if (!split)
    return;

  query = xaccMallocQuery ();

  xaccQuerySetBook (query, gnc_get_current_book ());

  xaccQueryAddGUIDMatch (query, xaccSplitGetGUID (split),
                         GNC_ID_SPLIT, QUERY_AND);

  id = report_helper (priv->ledger, split, query);
  if (id >= 0)
    reportWindow (id);
  LEAVE(" ");
}

/************************************************************/
/*                    Auxiliary functions                   */
/************************************************************/

void
gnc_plugin_page_register_set_options (GncPluginPage *plugin_page,
				      const char *lines_opt_page,
				      const char *lines_opt_name,
				      gint lines_default,
				      gint disallowCaps)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;
	priv->lines_opt_page = lines_opt_page;
	priv->lines_opt_name = lines_opt_name;
	priv->lines_default  = lines_default;
	priv->disallowCaps   = disallowCaps;
}

void
gnc_plugin_page_register_set_ui_description (GncPluginPage *plugin_page,
					     const char *ui_filename)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page));

	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;

	g_free(priv->ui_description);
	priv->ui_description = g_strdup(ui_filename);
}

GNCSplitReg *
gnc_plugin_page_register_get_gsr (GncPluginPage *plugin_page)
{
	GncPluginPageRegister *page;
	GncPluginPageRegisterPrivate *priv;

	g_return_val_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(plugin_page), NULL);

	page = GNC_PLUGIN_PAGE_REGISTER (plugin_page);
	priv = page->priv;

	return priv->gsr;
}

static void
gnc_plugin_page_help_changed_cb (GNCSplitReg *gsr, GncPluginPageRegister *register_page)
{
	GncPluginPageRegisterPrivate *priv;
	SplitRegister *reg;
	GncWindow *window;
	char *help;

	g_return_if_fail(GNC_IS_PLUGIN_PAGE_REGISTER(register_page));

	window = GNC_WINDOW(GNC_PLUGIN_PAGE(register_page)->window);
	if (!window) {
	  // This routine can be called before the page is added to a
	  // window.
	  return;
	}

	/* Get the text from the ledger */
	priv = register_page->priv;
	reg = gnc_ledger_display_get_split_register(priv->ledger);
	help = gnc_table_get_help(reg->table);
	gnc_window_set_status(window, GNC_PLUGIN_PAGE(register_page), help);
	g_free(help);
}

