/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "exifpanel.h"

#include "guiutils.h"
#include "rtimage.h"
#include "../rtengine/imagedata.h"
#include "../rtengine/metadata.h"

using namespace rtengine;
using namespace rtengine::procparams;


ExifPanel::ExifPanel () : idata (nullptr)
{
    for (auto &k : MetaDataParams::basicExifKeys) {
        editable_.push_back(std::make_pair(k, ""));
    }
    
    exifTree = Gtk::manage (new Gtk::TreeView());
    scrolledWindow = Gtk::manage (new Gtk::ScrolledWindow());

    exifTree->set_headers_visible (false);
    exifTree->set_rules_hint (false);
    exifTree->set_reorderable (false);
    exifTree->set_enable_search (true);
    exifTree->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    scrolledWindow->set_shadow_type(Gtk::SHADOW_NONE);
    scrolledWindow->set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
    scrolledWindow->property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);
    scrolledWindow->add(*exifTree);

    exifTreeModel = Gtk::TreeStore::create(exifColumns);
    exifTree->set_model(exifTreeModel);
    exifTree->set_grid_lines(Gtk::TREE_VIEW_GRID_LINES_NONE);
    exifTree->set_show_expanders(false);
    exifTree->set_tooltip_column(0);
    exifTree->set_enable_search(false);
        
    //keepicon = RTImage::createPixbufFromFile("tick-small.png");
    editicon = RTImage::createPixbufFromFile("edit-small.png");
    open_icon_ = RTImage::createPixbufFromFile("expander-open-small.png");
    closed_icon_ = RTImage::createPixbufFromFile("expander-closed-small.png");

    exif_active_renderer_.property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    exif_active_renderer_.signal_toggled().connect(sigc::mem_fun(this, &ExifPanel::onKeyActiveToggled));
    exif_active_column_.pack_start(exif_active_renderer_);
    exif_active_column_.set_cell_data_func(exif_active_renderer_, sigc::mem_fun(this, &ExifPanel::setKeyActive));
    
    exifTree->append_column(exif_active_column_);
    
    // {
    //     Gtk::TreeView::Column *c = Gtk::manage(new Gtk::TreeView::Column(""));
    //     Gtk::CellRendererPixbuf *pb = Gtk::manage(new Gtk::CellRendererPixbuf());
    //     c->pack_start(*pb, false);
    //     c->add_attribute(*pb, "pixbuf", exifColumns.expander_icon);
    //     exifTree->append_column(*c);
    // }
    
    Gtk::TreeView::Column *viewcol = Gtk::manage (new Gtk::TreeView::Column ("Field Name"));
    Gtk::CellRendererPixbuf* render_pb = Gtk::manage (new Gtk::CellRendererPixbuf ());
    Gtk::CellRendererText *render_txt = Gtk::manage (new Gtk::CellRendererText());
    render_txt->property_ellipsize() = Pango::ELLIPSIZE_END;
    viewcol->pack_start(*render_pb, false);
    viewcol->pack_start(*render_txt, true);
    viewcol->add_attribute(*render_pb, "pixbuf", exifColumns.icon);
    viewcol->add_attribute(*render_txt, "markup", exifColumns.label);
    viewcol->set_expand(true);
    viewcol->set_resizable(true);
    viewcol->set_fixed_width(35);
    viewcol->set_min_width(35);
    viewcol->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);

    render_pb->property_ypad() = 0;
    render_txt->property_ypad() = 0;
    render_pb->property_yalign() = 0;
    render_txt->property_yalign() = 0;

    exifTree->append_column(*viewcol);
    //exifTree->set_expander_column(*viewcol);

    Gtk::TreeView::Column *viewcolv = Gtk::manage(new Gtk::TreeView::Column ("Value"));
    Gtk::CellRendererText *render_txtv = Gtk::manage(new Gtk::CellRendererText());
    render_txtv->property_ellipsize() = Pango::ELLIPSIZE_END;
    viewcolv->pack_start (*render_txtv, true);
    viewcolv->add_attribute (*render_txtv, "markup", exifColumns.value);
    viewcolv->set_expand (true);
    viewcolv->set_resizable (true);
    viewcol->set_fixed_width (35);
    viewcolv->set_min_width (35);
    viewcolv->set_sizing (Gtk::TREE_VIEW_COLUMN_AUTOSIZE);

    render_txtv->property_ypad() = 0;

    exifTree->append_column(*viewcolv);

    pack_start (*scrolledWindow);

    Gtk::Grid* buttons1 = Gtk::manage (new Gtk::Grid());
    buttons1->set_row_homogeneous (true);
    buttons1->set_column_homogeneous (true);
    setExpandAlignProperties (buttons1, true, false, Gtk::ALIGN_FILL, Gtk::ALIGN_CENTER);

    const auto addbtn =
        [&](const Glib::ustring &tip, const Glib::ustring &icon1, const Glib::ustring &icon2=Glib::ustring()) -> Gtk::Button *
        {
            Gtk::Button *b = Gtk::manage(new Gtk::Button());
            b->set_image(*Gtk::manage(new RTImage(icon1, icon2)));
            b->set_tooltip_text(M(tip));
            b->get_style_context()->add_class("Right");
            setExpandAlignProperties(b, true, true, Gtk::ALIGN_FILL, Gtk::ALIGN_FILL);
            buttons1->attach_next_to(*b, Gtk::POS_RIGHT, 1, 1);
            return b;
        };

    activate_all_ = addbtn("EXIFPANEL_ACTIVATE_ALL_HINT", "tick.png");
    activate_none_ = addbtn("EXIFPANEL_ACTIVATE_NONE_HINT", "box.png");
    add = addbtn("EXIFPANEL_ADDEDIT", "edit.png");
    reset = addbtn("EXIFPANEL_RESETHINT", "undo.png", "redo.png");
    resetAll = addbtn("EXIFPANEL_RESETALLHINT", "undo-all.png", "redo-all.png");

    pack_end (*buttons1, Gtk::PACK_SHRINK);

    exifTree->get_selection()->signal_changed().connect (sigc::mem_fun (*this, &ExifPanel::exifSelectionChanged));
    exifTree->signal_row_activated().connect(sigc::mem_fun(*this, &ExifPanel::onExifRowActivated));

    reset->signal_clicked().connect ( sigc::mem_fun (*this, &ExifPanel::resetPressed) );
    resetAll->signal_clicked().connect ( sigc::mem_fun (*this, &ExifPanel::resetAllPressed) );
    add->signal_clicked().connect ( sigc::mem_fun (*this, &ExifPanel::addPressed) );
    activate_all_->signal_clicked().connect(sigc::mem_fun(*this, &ExifPanel::activateAllPressed));
    activate_none_->signal_clicked().connect(sigc::mem_fun(*this, &ExifPanel::activateNonePressed));

    exifTree->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &ExifPanel::onExifTreeClick));
    exifTree->signal_row_expanded().connect(sigc::mem_fun(*this, &ExifPanel::onExifRowExpanded));
    exifTree->signal_row_collapsed().connect(sigc::mem_fun(*this, &ExifPanel::onExifRowCollapsed));

    show_all ();
}


ExifPanel::~ExifPanel ()
{
}


void ExifPanel::read(const ProcParams* pp)
{
    disableListener();

    changeList = pp->metadata.exif;
    initial_active_keys_.clear();
    initial_active_keys_.insert(pp->metadata.exifKeys.begin(), pp->metadata.exifKeys.end());
    cur_active_keys_ = initial_active_keys_;
    setImageData(idata);
    refreshTags();

    enableListener();
}


void ExifPanel::write(ProcParams* pp)
{
    pp->metadata.exif = changeList;
    cur_active_keys_ = get_active_keys();
    pp->metadata.exifKeys.assign(cur_active_keys_.begin(), cur_active_keys_.end());
    std::sort(pp->metadata.exifKeys.begin(), pp->metadata.exifKeys.end());
}


void ExifPanel::setDefaults(const ProcParams* defParams)
{
    defChangeList = defParams->metadata.exif;
}


void ExifPanel::setImageData (const FramesMetaData* id)
{
    idata = id;
}


void ExifPanel::addTag(const std::string &key, const std::pair<Glib::ustring, Glib::ustring> &label, const Glib::ustring &value, bool editable, bool edited)
{
//    auto root = exifTreeModel->children();

    const auto getgroup =
        [&]() -> Gtk::TreeNodeChildren
        {
            auto root = exifTreeModel->children();
            
            // for (auto it = root.rbegin(), end = root.rend(); it != end; ++it) {
            //     auto row = *it;
            for (auto &row : root) {
                // auto row = *it;
                std::string key = row[exifColumns.key];
                if (row[exifColumns.is_group] && key == label.first) {
                    return row./*it->*/children();
                }
            }
            auto it = exifTreeModel->append(root);
            auto row = *it;
            
            row[exifColumns.editable] = false;
            row[exifColumns.edited] = false;
            row[exifColumns.key] = label.first;
            row[exifColumns.label] = "<b>" + label.first + "</b>";
            row[exifColumns.value_nopango] = "";
            row[exifColumns.value] = "";
            row[exifColumns.is_group] = true;

            return it->children();
        };

    auto root = getgroup();

    Gtk::TreeModel::Row row = *(exifTreeModel->append(root));
    row[exifColumns.editable] = editable;
    row[exifColumns.edited] = edited;
    row[exifColumns.key] = key;
    row[exifColumns.is_group] = false;
    //row[exifColumns.label] = label.second;
    row[exifColumns.value_nopango] = value;
    //row[exifColumns.value] = value;

    row[exifColumns.label] = escapeHtmlChars(label.second);
    row[exifColumns.value] = value;//escapeHtmlChars(value);

    bool active = all_keys_active() || cur_active_keys_.find(key) != cur_active_keys_.end();
    row[exifColumns.active] = active;

    if (edited) {
        row[exifColumns.icon] = editicon;
    // } else if (editable) {
    //     row[exifColumns.icon] = keepicon;
    }
}


void ExifPanel::refreshTags()
{
    Glib::RefPtr<Gtk::TreeSelection> selection = exifTree->get_selection();
    std::vector<Gtk::TreeModel::Path> sel = selection->get_selected_rows();
    
    exifTreeModel->clear();
    
    if (!idata) {
        return;
    }

    Glib::ustring fn = idata->getFileName();
    if (fn.empty()) {
        return;
    }

    std::unordered_set<std::string> ed;
    for (auto &p : editable_) {
        ed.insert(p.first);
    }

    const auto to_label =
        [](Exiv2::Exifdatum &tag) -> std::pair<Glib::ustring, Glib::ustring>
        {
            auto s = tag.key();
            auto pos = s.find('.');
            if (pos != std::string::npos) {
                s = s.substr(pos+1);
            }
            Glib::ustring g = "";
            pos = s.find('.');
            if (pos != std::string::npos) {
                g = s.substr(0, pos);
                // s = s.substr(pos+1);
            }
            s = tag.tagLabel();
            return std::make_pair(g, Glib::ustring(s));
        };

    try {
        rtengine::Exiv2Metadata meta(fn);
        meta.load();
        Exiv2::ExifData exif = meta.exifData();

        const auto to_value =
            [&](Exiv2::Exifdatum &tag) -> Glib::ustring
            {
                if (!tag.tagLabel().empty() && tag.typeId() != Exiv2::undefined &&
                    (tag.typeId() == Exiv2::asciiString || tag.size() < 256)) {
                    return escapeHtmlChars(tag.print(&exif));
                }
                return "<i>(Not shown)</i>";
            };

        if (const rtengine::FramesData *fd = dynamic_cast<const rtengine::FramesData *>(idata)) {
            fd->fillBasicTags(exif);
        }
        
        for (auto &p : changeList) {
            try {
                exif[p.first] = p.second;
            } catch (std::exception &exc) {
                std::cout << "ERROR: " << exc.what() << std::endl;
            }
        }

        for (auto &p : editable_) {
            Exiv2::ExifKey k(p.first);
            auto pos = exif.findKey(k);
            bool edited = false;
            Glib::ustring value = "";
            auto lbl = std::make_pair(M("EXIFPANEL_BASIC_GROUP"), k.tagLabel());
            p.second = k.tagLabel();
            if (pos != exif.end() && pos->size()) {
                edited = changeList.find(pos->key()) != changeList.end();
                value = pos->print(&exif);
            }
            addTag(p.first, lbl, value, true, edited);
        }
        std::set<std::string> keyset;
        for (auto &tag : exif) {
            bool editable = ed.find(tag.key()) != ed.end();
            if (!editable) {
                keyset.insert(tag.key());
            }
        }
        for (auto &k : keyset) {
            auto &tag = *(exif.findKey(Exiv2::ExifKey(k)));
            addTag(tag.key(), to_label(tag), to_value(tag), false, false);
        }
    } catch (std::exception &exc) {
        return;
    }

    for (auto &p : sel) {
        exifTree->get_selection()->select(p);
    }

    exifTree->expand_all();
}


void ExifPanel::exifSelectionChanged ()
{

    Glib::RefPtr<Gtk::TreeSelection> selection = exifTree->get_selection();
    std::vector<Gtk::TreeModel::Path> sel = selection->get_selected_rows();

    if (sel.size() > 1) {
        reset->set_sensitive (1);
    } else if (sel.size() == 1) {
        Gtk::TreeModel::iterator iter = exifTreeModel->get_iter (sel[0]);

        if (iter->get_value(exifColumns.icon) == editicon) {
            reset->set_sensitive (1);
        }
    } else {
        reset->set_sensitive (0);
    }
}


void ExifPanel::resetIt (Gtk::TreeModel::iterator  iter)
{
    if (!iter) {
        return;
    }

    auto key = iter->get_value(exifColumns.key);
    changeList.erase(key);
}


void ExifPanel::resetPressed()
{
    cur_active_keys_ = get_active_keys();

    std::vector<Gtk::TreeModel::Path> sel = exifTree->get_selection()->get_selected_rows();

    for (size_t i = 0; i < sel.size(); i++) {
        resetIt(exifTreeModel->get_iter(sel[i]));
    }

    refreshTags();
    notifyListener();
}


void ExifPanel::resetAllPressed ()
{
    setImageData(idata);
    changeList = defChangeList;
    cur_active_keys_ = initial_active_keys_;
    refreshTags();
    notifyListener();
}


void ExifPanel::addPressed ()
{

    Gtk::Dialog* dialog = new Gtk::Dialog (M ("EXIFPANEL_ADDTAGDLG_TITLE"), * ((Gtk::Window*)get_toplevel()), true);
    dialog->add_button (Gtk::Stock::OK, Gtk::RESPONSE_OK);
    dialog->add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

    Gtk::HBox* hb1 = new Gtk::HBox ();
    Gtk::HBox* hb2 = new Gtk::HBox ();

    Gtk::Label* tlabel = new Gtk::Label (M ("EXIFPANEL_ADDTAGDLG_SELECTTAG") + ":");
    MyComboBoxText* tcombo = new MyComboBoxText ();

    for (auto &p : editable_) {
        tcombo->append(p.second);
    }

    hb1->pack_start (*tlabel, Gtk::PACK_SHRINK, 4);
    hb1->pack_start (*tcombo);

    Gtk::Label* vlabel = new Gtk::Label (M ("EXIFPANEL_ADDTAGDLG_ENTERVALUE") + ":");
    Gtk::Entry* ventry = new Gtk::Entry ();
    hb2->pack_start (*vlabel, Gtk::PACK_SHRINK, 4);
    hb2->pack_start (*ventry);

    Glib::ustring sel;
    Glib::ustring val;
    {
        Glib::RefPtr<Gtk::TreeSelection> selection = exifTree->get_selection();
        std::vector<Gtk::TreeModel::Path> rows = selection->get_selected_rows();

        if (rows.size() == 1) {
            Gtk::TreeModel::iterator iter = exifTreeModel->get_iter(rows[0]);
            if (iter->get_value(exifColumns.editable)) {
                sel = iter->get_value(exifColumns.key);
                val = iter->get_value(exifColumns.value_nopango);
            }
        }
    }

    if (sel == "") {
        tcombo->set_active(0);
    } else {
        for (size_t i = 0; i < editable_.size(); ++i) {
            if (editable_[i].first == sel) {
                tcombo->set_active(i);
                break;
            }
        }
    }

    ventry->set_text(val);
    ventry->set_activates_default (true);
    dialog->set_default_response (Gtk::RESPONSE_OK);
    dialog->get_content_area()->pack_start (*hb1, Gtk::PACK_SHRINK);
    dialog->get_content_area()->pack_start (*hb2, Gtk::PACK_SHRINK, 4);
    tlabel->show ();
    tcombo->show ();
    vlabel->show ();
    ventry->show ();
    hb1->show ();
    hb2->show ();

    if (dialog->run () == Gtk::RESPONSE_OK) {
        cur_active_keys_ = get_active_keys();
        auto key = editable_[tcombo->get_active_row_number()].first;
        auto value = ventry->get_text();
        changeList[key] = value;
        if (!all_keys_active()) {
            cur_active_keys_.insert(key);
        }
        refreshTags();
        notifyListener();
    }

    delete dialog;
    delete tlabel;
    delete tcombo;
    delete vlabel;
    delete ventry;
    delete hb1;
    delete hb2;
}


void ExifPanel::activateAllPressed()
{
    disableListener();
    auto root = exifTreeModel->children();
    for (auto &group : root->children()) {
        group[exifColumns.active] = true;
        for (auto &row : group.children()) {
            row[exifColumns.active] = true;
        }
    }
    enableListener();
    notifyListener();
}


void ExifPanel::activateNonePressed()
{
    disableListener();
    auto root = exifTreeModel->children();
    for (auto &group : root->children()) {
        group[exifColumns.active] = false;
        for (auto &row : group.children()) {
            row[exifColumns.active] = false;
        }
    }
    enableListener();
    notifyListener();
}


void ExifPanel::notifyListener()
{
    if (listener) {
        listener->panelChanged(EvExif, M("HISTORY_CHANGED"));
    }
}


void ExifPanel::onKeyActiveToggled(const Glib::ustring &path)
{
    auto it = exifTreeModel->get_iter(path);
    if (it) {
        auto row = *it;
        bool b = !row[exifColumns.active];
        row[exifColumns.active] = b;
        if (row[exifColumns.is_group]) {
            for (auto &c : row.children()) {
                c[exifColumns.active] = b;
            }
        } else if (!b) {
            it = row.parent();
            if (it) {
                (*it)[exifColumns.active] = b;
            }
        }
        notifyListener();
    }
}


void ExifPanel::setKeyActive(Gtk::CellRenderer *renderer, const Gtk::TreeModel::iterator &it)
{
    auto row = *it;
    static_cast<Gtk::CellRendererToggle *>(renderer)->set_active(row[exifColumns.active]);
}


bool ExifPanel::all_keys_active() const
{
    return (cur_active_keys_.size() == 1 && *(cur_active_keys_.begin()) == "*");
}


std::unordered_set<std::string> ExifPanel::get_active_keys() const
{
    bool all_active = true;
    std::unordered_set<std::string> ret;
    auto root = exifTreeModel->children();
    for (auto &group : root->children()) {
        for (auto &entry : group.children()) {
            std::string key = entry[exifColumns.key];
            if (entry[exifColumns.active]) {
                ret.insert(key);
            } else {
                all_active = false;
            }
        }
    }
    if (all_active) {
        ret.clear();
        ret.insert("*");
    }
    return ret;
}

void ExifPanel::onExifTreeClick(GdkEventButton *event)
{    
    Gtk::TreeModel::Path pth;
    Gtk::TreeViewColumn *col;
    int cell_x;
    int cell_y;
    if (exifTree->get_path_at_pos(event->x, event->y, pth, col, cell_x, cell_y) && col == exifTree->get_column(1) && cell_x <= 22) {
        auto it = exifTreeModel->get_iter(pth);
        auto row = *it;
        if (row[exifColumns.is_group]) {
            if (exifTree->row_expanded(pth)) {
                exifTree->collapse_row(pth);
            } else {
                exifTree->expand_row(pth, false);
            }
        }
    }
}


void ExifPanel::onExifRowExpanded(const Gtk::TreeModel::iterator &it, const Gtk::TreeModel::Path &path)
{
    auto row = *it;
    if (row[exifColumns.is_group]) {
        row[exifColumns.icon] = open_icon_;
    }
}


void ExifPanel::onExifRowCollapsed(const Gtk::TreeModel::iterator &it, const Gtk::TreeModel::Path &path)
{
    auto row = *it;
    if (row[exifColumns.is_group]) {
        row[exifColumns.icon] = closed_icon_;
    }
}


void ExifPanel::onExifRowActivated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = exifTreeModel->get_iter(path);
    auto row = *it;
    if (row[exifColumns.editable]) {
        addPressed();
    }
}
