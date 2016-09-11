#include "dialogsearch.h"
#include "ui_dialogsearch.h"
#include "mainwindow.h"
#include "core.h"
#include "stringizer.h"

#include <command_codes.h>
#include <data.h>
#include <lmu_reader.h>
#include <functional>
#include <vector>
#include <tuple>
#include <QMessageBox>

DialogSearch::DialogSearch(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSearch)
{
    ui->setupUi(this);

    ui->list_result->setColumnCount(5);
    ui->list_result->setRowCount(0);
    ui->list_result->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->list_result->setEditTriggers(QAbstractItemView::NoEditTriggers);

    enableCache(false);
}

DialogSearch::~DialogSearch()
{
    delete ui;
}

void DialogSearch::updateUI()
{
    const QString format("%1: %2");

    for (auto &v : Data::variables)
        ui->combo_variable->addItem(format.arg(QString::number(v.ID), QString::fromStdString(v.name)));
    for (auto &s : Data::switches)
        ui->combo_switch->addItem(format.arg(QString::number(s.ID), QString::fromStdString(s.name)));
    for (auto &i : Data::items)
        ui->combo_item->addItem(format.arg(QString::number(i.ID), QString::fromStdString(i.name)));
    for (auto &e : Data::commonevents)
        ui->combo_eventname->addItem(format.arg(QString::number(e.ID), QString::fromStdString(e.name)));
}

void DialogSearch::enableCache(bool enable)
{
    useCache = enable;

    if (enable)
        map_cache = std::vector<std::shared_ptr<RPG::Map>>(Data::treemap.maps.size());
    else
        map_cache.clear();

}

void DialogSearch::on_button_search_clicked()
{
    objectData.clear();
    ui->list_result->clear();
    ui->list_result->setRowCount(0);
    QStringList sl;
    sl << "Map" << "Event" << "Event Page" << "Sourceline" << "Action";
    ui->list_result->setHorizontalHeaderLabels(sl);

    std::function<bool(const RPG::EventCommand&)> search_predicate;
    auto map_searcher = [&search_predicate](const RPG::Map& map) {
        std::vector<command_info> res;
        for (auto& e : map.events)
            for (auto& p : e.pages)
                for (size_t line = 0; line < p.event_commands.size(); line++)
                {
                    auto& com = p.event_commands[line];
                    if (search_predicate(com))
                        res.emplace_back(map.ID, e.ID, p.ID, line, com);
                }

        return res;
    };
    auto common_searcher = [&search_predicate](const std::vector<RPG::CommonEvent> events) {
        std::vector<command_info> res;
        for (auto& e : events)
            for (size_t line = 0; line < e.event_commands.size(); line++)
            {
                auto& com = e.event_commands[line];
                if (search_predicate(com))
                    res.emplace_back(0, e.ID, 0, line, com);
            }

        return res;
    };

    if (ui->radio_variable->isChecked())
    {
        //TODO implement me
    }
    else if (ui->radio_switch->isChecked())
    {
        //TODO implement me
    }
    else if (ui->radio_item->isChecked())
    {
        QRegularExpression re("^(\\d+):?.*$");
        int itemID = re.match(ui->combo_item->currentText()).captured(1).toInt();

        search_predicate = [itemID](const RPG::EventCommand& com)
        {
            switch (com.code)
            {
                case Cmd::ChangeItems:
                    return com.parameters[2] == itemID;
                case Cmd::ControlVars:
                    return com.parameters[4] == 4 && com.parameters[5] == itemID;
                case Cmd::ChangeEquipment:
                    return com.parameters[3] == 0 && com.parameters[4] == itemID;
                case Cmd::ConditionalBranch:
                    return com.parameters[0] == 4 && com.parameters[1] == itemID;
                default:
                    return false;
            }
        };
    }
    else //if (ui->radio_eventname->isChecked())
    {
        //TODO implement me
    }

    if (!search_predicate)
    {
        QMessageBox::warning(this, "", "This search parameter isn't supported yet.");
        return;
    }

    if (ui->scope_current->isChecked())
    {
        const auto mapID = static_cast<MainWindow*>(parent())->currentScene()->id();
        auto map = loadMap(mapID);

        map->ID = mapID; // FIX: currently XML serialization is broken
        auto res = map_searcher(*map);
        showResults(res);
    }
    else if (ui->scope_events->isChecked())
    {
        showResults(common_searcher(Data::commonevents));
    }
    else // if (ui->scope_project->isChecked())
    {
        for (auto &map : Data::treemap.maps)
        {
            ui->label_status->setText(QString("Parsing Map %1 / %2").arg(QString::number(map.ID + 1), QString::number(Data::treemap.maps.size())));
            QApplication::processEvents(); //FIXME: can this be done better?!

            auto mapp = loadMap(map.ID);

            if (!mapp) continue;

            mapp->ID = map.ID; // FIX: currently XML serialization is broken
            auto res = map_searcher(*mapp);
            showResults(res);
        }
    }
    ui->list_result->resizeColumnsToContents();
}

void DialogSearch::on_list_result_doubleClicked(const QModelIndex &index)
{
    auto *par = static_cast<MainWindow*>(parent());
    par->openScene(std::get<0>(objectData[index.row()]));
    auto *event = (*par->currentScene()->mapEvents())[std::get<1>(objectData[index.row()])];
    par->selectTile(event->x, event->y);
    par->currentScene()->centerOnTile(event->x, event->y);
}

std::shared_ptr<RPG::Map> DialogSearch::loadMap(int mapID)
{
    if (!(useCache && map_cache[mapID]))
    {
        const QString file = QString("Map%1.emu").arg(QString::number(mapID), 4, QLatin1Char('0'));
        const std::shared_ptr<RPG::Map> res_map{LMU_Reader::LoadXml(mCore->filePath(ROOT, file).toStdString())};

        if (useCache)
            map_cache[mapID] = res_map;

        return res_map;
    }

    return map_cache[mapID];
}

void DialogSearch::showResults(const std::vector<command_info>& results) {
    for (auto &r : results)
    {
        int mapID, eventID, eventPage,  sourceLine;
        const RPG::EventCommand& command = std::get<4>(r);
        std::tie(mapID, eventID, eventPage, sourceLine, std::ignore) = r;

        auto mm = mapID;
        QStringList maps_rev;
        if (mm == 0) // Common Event
        {
            maps_rev << "Common";
        }
        else
        {
            do
            {
                auto& mapinfo = Data::treemap.maps[mm];
                maps_rev << QString::fromStdString(mapinfo.name);
                mm = mapinfo.parent_map;
            } while (mm != 0);
        }

        const auto descr = Stringizer::stringize(command);

        const int rows = ui->list_result->rowCount();
        ui->list_result->setRowCount(rows+1);
        ui->list_result->setItem(rows, 0, new QTableWidgetItem(maps_rev.join(" > ")));
        ui->list_result->setItem(rows, 1, new QTableWidgetItem(QString::number(eventID)));
        ui->list_result->setItem(rows, 2, new QTableWidgetItem(QString::number(eventPage)));
        ui->list_result->setItem(rows, 3, new QTableWidgetItem(QString::number(sourceLine)));
        ui->list_result->setItem(rows, 4, new QTableWidgetItem(descr));

        objectData.push_back(r);
    }
}
