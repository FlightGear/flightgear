#include "CarriersLocationModel.hxx"

#include <algorithm>

#include <QPixmap>

#include "AIModel/AIManager.hxx"

CarriersLocationModel::CarriersLocationModel(QObject *parent)
    : QAbstractListModel(parent)
{
    SGPropertyNode_ptr localRoot(new SGPropertyNode);
    FGAIManager::registerScenarios(localRoot);

// this code encodes some scenario structure, sorry
    for (auto s : localRoot->getNode("sim/ai/scenarios")->getChildren("scenario")) {
        const std::string scenarioId = s->getStringValue("id");
        for (auto c : s->getChildren("carrier")) {
            processCarrier(scenarioId, c);
        }
    }
}

void CarriersLocationModel::processCarrier(const string &scenario, SGPropertyNode_ptr carrierNode)
{
    const auto name = QString::fromStdString(carrierNode->getStringValue("name"));
    const auto pennant = QString::fromStdString(carrierNode->getStringValue("pennant-number"));
    const auto tacan = QString::fromStdString(carrierNode->getStringValue("TACAN-channel-ID"));
    const auto desc = QString::fromStdString(carrierNode->getStringValue("description"));
    SGGeod geod = SGGeod::fromDeg(carrierNode->getDoubleValue("longitude"),
                                  carrierNode->getDoubleValue("latitude"));

    QStringList parkings;
    for (auto c : carrierNode->getChildren("parking-pos")) {
        parkings.append(QString::fromStdString(c->getStringValue()));
    }

    mCarriers.push_back(Carrier{
                            QString::fromStdString(scenario),
                            pennant,
                            name,
                            desc,
                            geod,
                            tacan,
                            parkings
                        });
}

int CarriersLocationModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return static_cast<int>(mCarriers.size());
}

QVariant CarriersLocationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto& c = mCarriers.at(static_cast<size_t>(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:          return c.mName;
   // case GeodRole:          return QVariant::fromValue(c.mInitialLocation);
    case IdentRole:         return c.mCallsign;
    case DescriptionRole:   return c.mDescription;
    case TypeRole:          return "Carrier";
    case IconRole:          return QPixmap(":/svg/aircraft-carrier");
    default:
        break;
    }

    return {};
}

QHash<int, QByteArray> CarriersLocationModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();

    result[GeodRole] = "geod";
    result[GuidRole] = "guid";
    result[IdentRole] = "ident";
    result[NameRole] = "name";
    result[IconRole] = "icon";
    result[TypeRole] = "type";
    result[DescriptionRole] = "description";
    result[NavFrequencyRole] = "frequency";
    return result;
}

int CarriersLocationModel::indexOf(const QString name) const
{
    auto it =  std::find_if(mCarriers.begin(), mCarriers.end(), [name]
                            (const Carrier& carrier)
    { return name == carrier.mName || name == carrier.mCallsign; });
    if (it == mCarriers.end())
        return -1;

    return static_cast<int>(std::distance(mCarriers.begin(), it));
}

SGGeod CarriersLocationModel::geodForIndex(int index) const
{
    const auto uIndex = static_cast<size_t>(index);
    if ((index < 0) || (uIndex >= mCarriers.size())) {
        return {};
    }

    const auto& c = mCarriers.at(uIndex);
    return c.mInitialLocation;
}

QString CarriersLocationModel::pennantForIndex(int index) const
{
    const auto uIndex = static_cast<size_t>(index);
    if ((index < 0) || (uIndex >= mCarriers.size())) {
        return {};
    }

    const auto& c = mCarriers.at(uIndex);
    return c.mCallsign;
}

QStringList CarriersLocationModel::parkingsForIndex(int index) const
{
    const auto uIndex = static_cast<size_t>(index);
    if ((index < 0) || (uIndex >= mCarriers.size())) {
        return {};
    }

    const auto& c = mCarriers.at(uIndex);
    return c.mParkings;
}
