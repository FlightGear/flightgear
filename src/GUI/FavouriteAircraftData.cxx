#include "FavouriteAircraftData.hxx"

#include <QSettings>

#include <memory>

static std::unique_ptr<FavouriteAircraftData> static_instance;

FavouriteAircraftData *FavouriteAircraftData::instance()
{
    if (!static_instance) {
        static_instance.reset(new FavouriteAircraftData);
    }

    return static_instance.get();
}

bool FavouriteAircraftData::isFavourite(QUrl u) const
{
     return m_favourites.contains(u);
}

bool FavouriteAircraftData::setFavourite(QUrl u, bool b)
{
    const auto cur = m_favourites.contains(u);
    if (b == cur)
        return false;

    if (b && !cur) {
        m_favourites.append(u);
    } else if (!b && cur) {
        m_favourites.removeOne(u);
    }

    emit changed(u);
    saveFavourites();
    return true;
}

FavouriteAircraftData::FavouriteAircraftData()
{
    loadFavourites();
}

void FavouriteAircraftData::loadFavourites()
{
    m_favourites.clear();
    QSettings settings;
    Q_FOREACH(auto v, settings.value("favourite-aircraft").toList()) {
        m_favourites.append(v.toUrl());
    }
}

void FavouriteAircraftData::saveFavourites()
{
    QVariantList favs;
    Q_FOREACH(auto u, m_favourites) {
        favs.append(u);
    }
    QSettings settings;
    settings.setValue("favourite-aircraft", favs);
}
