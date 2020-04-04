#ifndef FAVOURITEAIRCRAFTDATA_HXX
#define FAVOURITEAIRCRAFTDATA_HXX

#include <QObject>
#include <QUrl>
#include <QVector>

class FavouriteAircraftData : public QObject
{
    Q_OBJECT
public:
    static FavouriteAircraftData* instance();

    bool isFavourite(QUrl u) const;

    bool setFavourite(QUrl u, bool b);

signals:
   void changed(QUrl u);

private:
    FavouriteAircraftData();

    void loadFavourites();
    void saveFavourites();

    QVector<QUrl> m_favourites;

};

#endif // FAVOURITEAIRCRAFTDATA_HXX
