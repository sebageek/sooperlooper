/*
** Copyright (C) 2004 Jesse Chappell <jesse@essej.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**  
*/

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>

#include <iostream>
#include <string>
#include <vector>
#include <list>

#include "gui_frame.hpp"
#include "midi_bind_dialog.hpp"
#include "loop_control.hpp"
#include "gui_app.hpp"
#include "keyboard_target.hpp"

#include <midi_bind.hpp>
#include <command_map.hpp>

using namespace SooperLooperGui;
using namespace SooperLooper;
using namespace std;

enum {
	ID_ListCtrl = 8000,
	ID_CloseButton,
	ID_LearnButton,
	ID_ControlCombo,
	ID_LoopNumCombo,
	ID_ChanSpin,
	ID_TypeCombo,
	ID_ParamSpin,
	ID_LBoundCtrl,
	ID_UBoundCtrl,
	ID_StyleCombo,
	ID_AddButton,
	ID_ModifyButton,
	ID_RemoveButton,
	ID_ClearAllButton,
	ID_LoadButton,
	ID_SaveButton

};

BEGIN_EVENT_TABLE(SooperLooperGui::MidiBindDialog, wxFrame)
	EVT_CLOSE(SooperLooperGui::MidiBindDialog::on_close)
//EVT_LIST_ITEM_ACTIVATED (ID_ListCtrl, SooperLooperGui::MidiBindDialog::list_event)
	EVT_LIST_ITEM_SELECTED (ID_ListCtrl, SooperLooperGui::MidiBindDialog::item_selected)

	EVT_BUTTON (ID_CloseButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_LearnButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_AddButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_RemoveButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_ModifyButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_ClearAllButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_LoadButton, SooperLooperGui::MidiBindDialog::on_button)
	EVT_BUTTON (ID_SaveButton, SooperLooperGui::MidiBindDialog::on_button)

	EVT_COMBOBOX(ID_ControlCombo, SooperLooperGui::MidiBindDialog::on_combo)
	
	EVT_SIZE (SooperLooperGui::MidiBindDialog::onSize)
	EVT_PAINT (SooperLooperGui::MidiBindDialog::onPaint)
	
END_EVENT_TABLE()

static const wxString CcString("CC");
static const wxString NoteString("Note");
static const wxString PcString("PC");
	
static int wxCALLBACK list_sort_callback (long item1, long item2, long sortData)
{

	MidiBindInfo * info1 = (MidiBindInfo*)(item1);
	MidiBindInfo * info2 = (MidiBindInfo*)(item2);

	return info1->control > info2->control;
}

	
// ctor(s)
MidiBindDialog::MidiBindDialog(GuiFrame * parent, wxWindowID id, const wxString& title,
		       const wxPoint& pos,
		       const wxSize& size,
		       long style ,
		       const wxString& name)

	: wxFrame ((wxWindow *)parent, id, title, pos, size, style, name), _parent (parent)
{
	_learning = false;
	_justResized = false;
	_selitem = -1;
	init();
}

MidiBindDialog::~MidiBindDialog()
{

}

void MidiBindDialog::onSize(wxSizeEvent &ev)
{

	_justResized = true;
	ev.Skip();
}

void MidiBindDialog::onPaint(wxPaintEvent &ev)
{
	if (_justResized) {
		int width,height, cwidth;

		_justResized = false;
		
		_listctrl->GetClientSize(&width, &height);

		cwidth = _listctrl->GetColumnWidth(0);
		_listctrl->SetColumnWidth(3, width-cwidth);
		
	}

	ev.Skip();
}


void MidiBindDialog::init()
{
	wxBoxSizer * topsizer = new wxBoxSizer(wxVERTICAL);

	_listctrl = new wxListCtrl(this, ID_ListCtrl, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_SINGLE_SEL|wxSUNKEN_BORDER);
	_listctrl->InsertColumn(0, wxT("Control"));
	_listctrl->InsertColumn(1, wxT("Loop#"));
	_listctrl->InsertColumn(2, wxT("Midi Event"));
	_listctrl->InsertColumn(3, wxT("Range"));

	_listctrl->SetColumnWidth(0, 180);
	_listctrl->SetColumnWidth(1, 60);
	_listctrl->SetColumnWidth(2, 100);
	
	

	wxBoxSizer * buttsizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton *butt = new wxButton (this, ID_LoadButton, wxT("Load..."));
	buttsizer->Add (butt, 0, wxALL|wxALIGN_CENTRE, 3);

	butt = new wxButton (this, ID_SaveButton, wxT("Save..."));
	buttsizer->Add (butt, 0, wxALL|wxALIGN_CENTRE, 3);

	buttsizer->Add (1,-1,1, wxALL, 0);
	
	butt = new wxButton (this, ID_ClearAllButton, wxT("Clear All"));
	buttsizer->Add (butt, 0, wxALL|wxALIGN_RIGHT, 3);
	
	topsizer->Add (buttsizer, 0, wxLEFT|wxTOP|wxRIGHT|wxEXPAND, 4);

	// add list
	topsizer->Add (_listctrl, 1, wxEXPAND|wxALL, 4);


	
	_edit_panel = new wxPanel(this, -1);
	wxBoxSizer * editsizer = new wxBoxSizer(wxHORIZONTAL);

	wxStaticBox * shotBox = new wxStaticBox(_edit_panel, -1, "Command/Control", wxDefaultPosition, wxDefaultSize);
        wxStaticBoxSizer * colsizer = new wxStaticBoxSizer(shotBox, wxVERTICAL);
	
	//wxBoxSizer * colsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * rowsizer;
	wxStaticText * staticText;
	//colsizer->Add (staticText, 0, wxALL|wxALIGN_CENTRE, 2);

	_control_combo = new wxComboBox(_edit_panel, ID_ControlCombo, wxT(""),  wxDefaultPosition, wxSize(100, -1), 0, 0, wxCB_DROPDOWN|wxCB_READONLY);
	//_control_combo->SetToolTip(wxT("Choose control or command"));
	populate_controls();
	
	colsizer->Add (_control_combo, 0, wxALL|wxALIGN_CENTRE|wxEXPAND, 2);

	rowsizer = new wxBoxSizer(wxHORIZONTAL);
	staticText = new wxStaticText(_edit_panel, -1, "Loop #", wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT);
	rowsizer->Add (staticText, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	_loopnum_combo =  new wxComboBox(_edit_panel, ID_LoopNumCombo, wxT(""),  wxDefaultPosition, wxSize(100, -1), 0, 0, wxCB_DROPDOWN|wxCB_READONLY);
	_loopnum_combo->Append (wxT("All"), (void *) 0);
	for (int i=1; i <= 16; ++i) {
		_loopnum_combo->Append (wxString::Format("%d", i), (void *) i);
	}
	rowsizer->Add (_loopnum_combo, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 1);

	colsizer->Add (rowsizer, 0, wxALL|wxEXPAND, 0);

	editsizer->Add (colsizer, 0, wxALL|wxEXPAND, 4);

	
	
	// midi event stuff
	shotBox = new wxStaticBox(_edit_panel, -1, "MIDI Event", wxDefaultPosition, wxDefaultSize);
        colsizer = new wxStaticBoxSizer(shotBox, wxVERTICAL);
	
	//staticText = new wxStaticText(_edit_panel, -1, "MIDI Event", wxDefaultPosition, wxSize(-1, -1), wxALIGN_CENTRE);
	//colsizer->Add (staticText, 0, wxALL|wxALIGN_CENTRE, 2);

	rowsizer = new wxBoxSizer(wxHORIZONTAL);
	staticText = new wxStaticText(_edit_panel, -1, "Ch#", wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT);
	rowsizer->Add (staticText, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	_chan_spin =  new wxSpinCtrl(_edit_panel, ID_ChanSpin, "1", wxDefaultPosition, wxSize(50,-1), wxSP_ARROW_KEYS, 1, 16, 1, wxT("KeyAware"));
	rowsizer->Add (_chan_spin, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);
	
	_type_combo = new wxComboBox(_edit_panel, ID_TypeCombo, wxT(""),  wxDefaultPosition, wxSize(100, -1), 0, 0, wxCB_DROPDOWN|wxCB_READONLY);
	//_control_combo->SetToolTip(wxT("Choose control or command"));
	_type_combo->Append (NoteString);
	_type_combo->Append (CcString);
	_type_combo->Append (PcString);
	rowsizer->Add (_type_combo, 1, wxALL|wxALIGN_CENTRE_VERTICAL|wxEXPAND, 2);

	_param_spin =  new wxSpinCtrl(_edit_panel, ID_ParamSpin, "0", wxDefaultPosition, wxSize(50,-1), wxSP_ARROW_KEYS, 0, 127, 0, wxT("KeyAware"));
	rowsizer->Add (_param_spin, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	_learn_button = new wxButton (_edit_panel, ID_LearnButton, wxT("Learn"), wxDefaultPosition, wxSize(-1, -1));
	rowsizer->Add (_learn_button, 0, wxALL, 3);

	colsizer->Add (rowsizer, 0, wxALL|wxEXPAND, 0);

	_range_panel = new wxPanel(_edit_panel);
	rowsizer = new wxBoxSizer(wxHORIZONTAL);
	staticText = new wxStaticText(_range_panel, -1, "Range", wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT);
	rowsizer->Add (staticText, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	_lbound_ctrl = new wxTextCtrl(_range_panel, ID_LBoundCtrl, "", wxDefaultPosition, wxSize(40, -1), 0, wxDefaultValidator, wxT("KeyAware"));
	rowsizer->Add (_lbound_ctrl, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 1);

	staticText = new wxStaticText(_range_panel, -1, " to ", wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT);
	rowsizer->Add (staticText, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 2);

	_ubound_ctrl = new wxTextCtrl(_range_panel, ID_UBoundCtrl, "", wxDefaultPosition, wxSize(40, -1), 0, wxDefaultValidator, wxT("KeyAware"));
	rowsizer->Add (_ubound_ctrl, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 1);

	_style_combo =  new wxComboBox(_range_panel, ID_StyleCombo, wxT(""),  wxDefaultPosition, wxSize(30, -1), 0, 0, wxCB_DROPDOWN|wxCB_READONLY);
	_style_combo->Append (wxT("Normal"), (void *) MidiBindInfo::NormalStyle);
	_style_combo->Append (wxT("Gain"), (void *) MidiBindInfo::GainStyle);
	_style_combo->SetToolTip(wxT("Choose a scaling type"));
	rowsizer->Add (_style_combo, 1, wxALL|wxALIGN_CENTRE_VERTICAL, 1);

	_range_panel->SetAutoLayout( true );     // tell dialog to use sizer
        _range_panel->SetSizer( rowsizer );      // actually set the sizer
	rowsizer->SetSizeHints( _range_panel );   // set size hints to honour mininum size
	rowsizer->Fit(_range_panel);
	
	colsizer->Add (_range_panel, 0, wxALL|wxEXPAND, 0);

	
	editsizer->Add (colsizer, 1, wxALL|wxEXPAND, 4);

	
	
	
	_edit_panel->SetAutoLayout( true );     // tell dialog to use sizer
	_edit_panel->SetSizer( editsizer );      // actually set the sizer
	editsizer->SetSizeHints( _edit_panel );   // set size hints to honour mininum size
	editsizer->Fit(_edit_panel);
	
	topsizer->Add (_edit_panel, 0, wxEXPAND|wxALL, 1);

	buttsizer = new wxBoxSizer(wxHORIZONTAL);
	buttsizer->Add (1,1, 1, wxALL, 0);

	butt = new wxButton (this, ID_AddButton, wxT("Add New"));
	buttsizer->Add (butt, 0, wxALL|wxALIGN_CENTRE, 3);
	
	butt = new wxButton (this, ID_RemoveButton, wxT("Remove"));
	buttsizer->Add (butt, 0, wxALL|wxALIGN_CENTRE, 3);

	buttsizer->Add (10, 1, 0);
	
	butt = new wxButton (this, ID_ModifyButton, wxT("Modify"));
	buttsizer->Add (butt, 0, wxALL|wxALIGN_CENTRE, 3);


	//buttsizer->Add (1,-1,1, wxALL, 0);
	
	
	//butt = new wxButton (this, ID_CloseButton, wxT("Close"));
	//buttsizer->Add (butt, 0, wxALL|wxALIGN_CENTRE, 3);


	topsizer->Add (buttsizer, 0, wxALL|wxEXPAND|wxALIGN_RIGHT, 4);



	_parent->get_loop_control().MidiBindingChanged.connect (slot (*this, &MidiBindDialog::got_binding_changed));
	_parent->get_loop_control().ReceivedNextMidi.connect (slot (*this, &MidiBindDialog::recvd_next_midi));
	_parent->get_loop_control().NextMidiCancelled.connect (slot (*this, &MidiBindDialog::cancelled_next_midi));

	refresh_state();
	
	this->SetAutoLayout( true );     // tell dialog to use sizer
	this->SetSizer( topsizer );      // actually set the sizer
	topsizer->Fit( this );            // set size to minimum size as calculated by the sizer
	topsizer->SetSizeHints( this );   // set size hints to honour mininum size
}

void MidiBindDialog::populate_controls()
{
	// add all known controls to combo
	CommandMap & cmap = CommandMap::instance();
	
	cmap.get_commands(_cmdlist);
	cmap.get_controls(_cmdlist);

	for (list<string>::iterator iter = _cmdlist.begin(); iter != _cmdlist.end(); ++iter)
	{
		if (cmap.is_command(*iter)) {
			_control_combo->Append (wxString::Format("[cmd]  %s", iter->c_str()), (void *) (iter->c_str()));
		}
		else if (cmap.is_input_control(*iter)) {
			_control_combo->Append (wxString::Format("[ctrl]  %s", iter->c_str()), (void *) (iter->c_str()));
		}
	}
	
	
}

void MidiBindDialog::cancelled_next_midi()
{
	_learning = false;
	_learn_button->SetLabel (wxT("Learn"));
	_learn_button->SetForegroundColour (*wxBLACK); // todo default
}

void MidiBindDialog::recvd_next_midi(SooperLooper::MidiBindInfo & info)
{
	if (_learning) {
		cerr << "got next: " << info.serialize() << endl;
		MidiBindInfo ninfo = _currinfo;
		ninfo.channel = info.channel;
		ninfo.param = info.param;
		ninfo.type = info.type;

		update_entry_area (&ninfo);
		
		_learn_button->SetLabel (wxT("Learn"));
		_learn_button->SetForegroundColour (*wxBLACK); // todo default
		
		_learning = false;
	}

}

void MidiBindDialog::got_binding_changed(SooperLooper::MidiBindInfo & info)
{
	// cancel learning too
	if (_learning) {
		_learn_button->SetLabel (wxT("Learn"));
		_learn_button->SetForegroundColour (*wxBLACK); // todo default
		_learning = false;
	}

	refresh_state();
}

void MidiBindDialog::refresh_state()
{
	_listctrl->DeleteAllItems();

	wxListItem item;
	item.SetMask (wxLIST_MASK_TEXT|wxLIST_MASK_DATA|wxLIST_MASK_STATE);
	item.SetColumn(0);
	item.SetWidth(wxLIST_AUTOSIZE);
	item.SetStateMask(wxLIST_STATE_SELECTED);
	item.SetState(0);

	_bind_list.clear();

	
	_parent->get_loop_control().midi_bindings().get_bindings(_bind_list);
	
	int itemid = 0;
	
	for (MidiBindings::BindingList::iterator biter = _bind_list.begin(); biter != _bind_list.end(); ++biter)
	{
		MidiBindInfo & info = (*biter);
		
		// control
		item.SetId(itemid);
		item.SetColumn(0);
		item.SetData((void *) &info);
		item.SetText (wxString::FromAscii (info.control.c_str()));

		_listctrl->InsertItem (item);

		// loop #
		item.SetColumn(1);
		item.SetText (wxString::Format(wxT("%d"), info.instance + 1));
		if (info.instance < 0) {
			item.SetText (wxT("All"));
		}
		_listctrl->SetItem (item);

		// midi event
		item.SetColumn(2);
		item.SetText (wxString::Format(wxT("ch%d - %s - %d"), info.channel+1, info.type.c_str(), info.param));
		_listctrl->SetItem (item);

		// range
		item.SetColumn(3);
		if (info.command == "set") {
			item.SetText (wxString::Format(wxT("%g - %g %s"), info.lbound, info.ubound, info.style == MidiBindInfo::GainStyle ? wxT("gain") : wxT("")));
		}
		else {
			item.SetText(wxT(""));
		}
		_listctrl->SetItem (item);
		

		itemid++;
	}

	_listctrl->SortItems (list_sort_callback, (unsigned) _listctrl);
	
	for (long i=0; i < _listctrl->GetItemCount(); ++i) {
		item.SetId(i);
		item.SetColumn(0);
		if (_listctrl->GetItem(item)) {
			MidiBindInfo * info = (MidiBindInfo *) item.GetData();
			if (*info == _currinfo) {
				_listctrl->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
				_listctrl->EnsureVisible(i);
				_selitem  = i;
				// cerr << "matched curringo " << i << endl;
				break;
			}
		}
	}
	

// 	if (selexists) {
// 		_edit_panel->Enable(true);
// 	} else {
// 		_edit_panel->Enable(false);
// 	}
	
}


void MidiBindDialog::update_entry_area(MidiBindInfo * usethis)
{
	if (_selitem < 0 && !usethis) {
		return;
	}

	MidiBindInfo * info = static_cast<MidiBindInfo *>((void *)_listctrl->GetItemData(_selitem));

	if (usethis) {
		info = usethis;
	}

	if (!info) {
		return;
	}

	for (int i=0; i < _control_combo->GetCount(); ++i) {
		if (static_cast<const char *>((void *)_control_combo->GetClientData(i)) == info->control) {
			_control_combo->SetSelection(i);
			break;
		}
	}

	_loopnum_combo->SetSelection(info->instance + 1);
	_chan_spin->SetValue(info->channel + 1);

	if (info->type == "cc") {
		_type_combo->SetStringSelection(CcString);
	}
	else if (info->type == "n") {
		_type_combo->SetStringSelection(NoteString);
	}
	else if (info->type == "pc") {
		_type_combo->SetStringSelection(PcString);
	}

	if (info->command == "set") {
		_range_panel->Enable(true);
	}
	else {
		_range_panel->Enable(false);
	}
	
	_param_spin->SetValue(info->param);


	_lbound_ctrl->SetValue (wxString::Format("%g", info->lbound));
	_ubound_ctrl->SetValue (wxString::Format("%g", info->ubound));

	if (info->style == MidiBindInfo::NormalStyle) {
		_style_combo->SetSelection(0);
	}
	else {
		_style_combo->SetSelection(1);
	}
	
}

void MidiBindDialog::update_curr_binding()
{
	double fval;
	CommandMap & cmap = CommandMap::instance();
	
	// take info from editpanel and set the MidiBindInfo
	_currinfo.control = static_cast<const char *>((void *) _control_combo->GetClientData(_control_combo->GetSelection()));
	_currinfo.channel = _chan_spin->GetValue() - 1;
	_currinfo.instance = (int)(long) _loopnum_combo->GetClientData(_loopnum_combo->GetSelection()) - 1;

	
	wxString tsel = _type_combo->GetStringSelection();
	if (tsel == NoteString) {
		_currinfo.type = "n";
	}
	else if (tsel == CcString) {
		_currinfo.type = "cc";
	}
	else if (tsel == PcString) {
		_currinfo.type = "pc";
	}

	if (cmap.is_command(_currinfo.control)) {
		if (_currinfo.type == "n") {
			_currinfo.command = "note";
		}
		else {
			_currinfo.command = "down";
		}
	}
	else {
		// control
		_currinfo.command = "set";
	}

	
	_currinfo.param = _param_spin->GetValue();

	if (_lbound_ctrl->GetValue().ToDouble(&fval)) {
		_currinfo.lbound = (float) fval;
	}
	else {
		_currinfo.lbound = 0.0;
	}

	if (_ubound_ctrl->GetValue().ToDouble(&fval)) {
		_currinfo.ubound = (float) fval;
	}
	else {
		_currinfo.ubound = 1.0;
	}

	if (_style_combo->GetSelection() == 1) {
		_currinfo.style = MidiBindInfo::GainStyle;
	}
	else {
		_currinfo.style = MidiBindInfo::NormalStyle;
	}
	
}

void MidiBindDialog::on_combo (wxCommandEvent &ev)
{
	string control = static_cast<const char *>((void *)_control_combo->GetClientData(_control_combo->GetSelection()));

 	if (CommandMap::instance().is_input_control(control)) {
 		_range_panel->Enable(true);
 	}
 	else {
 		_range_panel->Enable(false);
 	}
}

void MidiBindDialog::on_button (wxCommandEvent &ev)
{
	if (ev.GetId() == ID_CloseButton) {
		Show(false);
	}
	else if (ev.GetId() == ID_AddButton)
	{
		update_curr_binding();
		_parent->get_loop_control().add_midi_binding(_currinfo);
		_parent->get_loop_control().request_all_midi_bindings();
	}
	else if (ev.GetId() == ID_LearnButton) {
		if (!_learning) {
			update_curr_binding();
			
			_learn_button->SetLabel (wxT("C. Learn"));
			_learn_button->SetForegroundColour (*wxRED);
			_learning = true;
			_parent->get_loop_control().request_next_midi_event();
		}
		else {
			//_learn_button->SetLabel (wxT("Learn"));
			//_learn_button->SetForegroundColour (*wxBLACK); // todo default
			//_learning = false;
			
			_parent->get_loop_control().cancel_next_midi_event();
		}
	}
	else if (ev.GetId() == ID_RemoveButton)
	{
		if (_listctrl->GetSelectedItemCount() > 0) {
			update_curr_binding();
			_parent->get_loop_control().remove_midi_binding(_currinfo);
			_parent->get_loop_control().request_all_midi_bindings();
		}
	}
	else if (ev.GetId() == ID_ModifyButton)
	{
		//if (_listctrl->GetSelectedItemCount() > 0) {
			_parent->get_loop_control().remove_midi_binding(_currinfo);
			update_curr_binding();
			_parent->get_loop_control().add_midi_binding(_currinfo);
			_parent->get_loop_control().request_all_midi_bindings();
			//}
	}
	else if (ev.GetId() == ID_ClearAllButton)
	{
		_parent->get_loop_control().clear_midi_bindings();
	}
	else if (ev.GetId() == ID_LoadButton)
	{
		if (_parent->get_loop_control().is_engine_local()) {
			
			::wxGetApp().getFrame()->get_keyboard().set_enabled(false);
			wxString filename = wxFileSelector(wxT("Choose midi binding file to open"), wxT(""), wxT(""), wxT(""), wxT("*.slb"), wxOPEN|wxCHANGE_DIR);
			::wxGetApp().getFrame()->get_keyboard().set_enabled(true);

			if ( !filename.empty() )
			{
				_parent->get_loop_control().load_midi_bindings(filename.c_str());
			}
		}
		else {
			// popup basic filename text entry
			::wxGetApp().getFrame()->get_keyboard().set_enabled(false);

			wxString filename = ::wxGetTextFromUser(wxString::Format("Specify midi binding file to load on remote host '%s'",
										 _parent->get_loop_control().get_engine_host().c_str())
								, wxT("Load Midi Binding"));

			::wxGetApp().getFrame()->get_keyboard().set_enabled(true);

			if (!filename.empty()) {
				_parent->get_loop_control().load_midi_bindings(filename.c_str());
			}
		}
			
	}
	else if (ev.GetId() == ID_SaveButton)
	{
		if (_parent->get_loop_control().is_engine_local()) {
			
			::wxGetApp().getFrame()->get_keyboard().set_enabled(false);
			wxString filename = wxFileSelector(wxT("Choose midi binding file to save"), wxT(""), wxT(""), wxT(""), wxT("*.slb"), wxSAVE|wxCHANGE_DIR);
			::wxGetApp().getFrame()->get_keyboard().set_enabled(true);

			if ( !filename.empty() )
			{
				_parent->get_loop_control().save_midi_bindings(filename.c_str());
			}
		}
		else {
			// popup basic filename text entry
			::wxGetApp().getFrame()->get_keyboard().set_enabled(false);

			wxString filename = ::wxGetTextFromUser(wxString::Format("Specify midi binding file to save on remote host '%s'",
										 _parent->get_loop_control().get_engine_host().c_str())
								, wxT("Save Midi Binding"));

			::wxGetApp().getFrame()->get_keyboard().set_enabled(true);

			if (!filename.empty()) {
				_parent->get_loop_control().save_midi_bindings(filename.c_str());
			}
		}
	}
	else {
		ev.Skip();
	}
}
				   
void MidiBindDialog::on_close (wxCloseEvent &ev)
{
	if (!ev.CanVeto()) {
		
		Destroy();
	}
	else {
		ev.Veto();
		
		Show(false);
	}
}


void MidiBindDialog::item_selected (wxListEvent & ev)
{
	//cerr << "item " << ev.GetText() << " sel" << endl;
	_selitem = ev.GetIndex();

	update_entry_area();
	update_curr_binding();
	
}

void MidiBindDialog::learning_stopped ()
{
	// cerr << "learning stopped " << endl;
	//_learn_button->SetLabel (wxT("Learn Selected"));
	//_learn_button->SetForegroundColour (*wxBLACK);

	//refresh_state();

	//_listctrl->SetFocus();
}
	
	
