#include "LauncherArgumentTokenizer.hxx"

#include <algorithm>
#include <set>

#include <QVariantMap>
#include <QJSEngine>

LauncherArgumentTokenizer::LauncherArgumentTokenizer()
{

}

void LauncherArgumentTokenizer::tokenize(QString in)
{
    m_tokens.clear();
    m_valid = false;

    int index = 0;
    const int len = in.count();
    QChar c, nc;
    State state = Start;
    QString key, value;
    std::vector<ArgumentToken> result;

    for (; index < len; ++index) {
        c = in.at(index);
        nc = index < (len - 1) ? in.at(index + 1) : QChar();

        switch (state) {
        case Start:
            if (c == QChar('-')) {
                if (nc == QChar('-')) {
                    state = Key;
                    key.clear();
                    ++index;
                } else {
                    // should we pemit single hyphen arguments?
                    // choosing to fail for now
                    return;
                }
            } else if (c == QChar('#')) {
                state = Comment;
                break;
            } else if (c.isSpace()) {
                break;
            }
            break;

        case Key:
            if (c == QChar('=')) {
                state = Value;
                value.clear();
            } else if (c.isSpace()) {
                state = Start;
                result.emplace_back(key);
            } else {
                // could check for illegal charatcers here
                key.append(c);
            }
            break;

        case Value:
            if (c == QChar('"')) {
                state = Quoted;
            } else if (c.isSpace()) {
                state = Start;
                result.emplace_back(key, value);
            } else {
                value.append(c);
            }
            break;

        case Quoted:
            if (c == QChar('\\')) {
                // check for escaped double-quote inside quoted value
                if (nc == QChar('"')) {
                    ++index;
                } else {
                    value.append(c); // normal '\' inside quoted
                }
            } else if (c == QChar('"')) {
                state = Value;
            } else {
                value.append(c);
            }
            break;

        case Comment:
            if ((c == QChar('\n')) || (c == QChar('\r'))) {
                state = Start;
                break;
            } else {
                // nothing to do, eat comment chars
            }
            break;
        } // of state switch
    } // of character loop

    // ensure last argument isn't lost
    if (state == Key) {
        result.emplace_back(key);
    } else if (state == Value) {
        result.emplace_back(key, value);
    }

    m_valid = true;
    m_tokens = result;
}

QVariantList LauncherArgumentTokenizer::tokens() const
{
    if (!m_valid)
        return {};

    QVariantList result;
    for (auto tk : m_tokens) {
        QVariantMap m;
        m["arg"] = tk.arg;
        m["value"] = tk.value;
        result.append(m);
    }
    return result;
}

bool LauncherArgumentTokenizer::isValid() const
{
    return m_valid;
}

const std::set<std::string> unsupportedArgs({
    "fg-aircraft", "fg-root", "fg-home", "aircraft-dir"});

bool LauncherArgumentTokenizer::haveUnsupportedArgs() const
{
    return haveArgsIn(unsupportedArgs);
}

void LauncherArgumentTokenizer::setArgString(QString argString)
{
    if (m_argString == argString)
        return;

    m_argString = argString;
    tokenize(m_argString);
    emit argStringChanged(m_argString);
}

const std::set<std::string> positionalArgs({
    "lat", "lon", "vor", "ndb", "fix", "parkpos",
    "airport", "parking-id", "runway", "carrier", "carrier-position"
});

bool LauncherArgumentTokenizer::haveArgsIn(const std::set<std::string>& args) const
{
    if (!m_valid)
        return false;

    auto n = std::count_if(m_tokens.begin(), m_tokens.end(),
                           [&args](const ArgumentToken& tk)
    {
        return (args.find(tk.arg.toStdString()) != args.end());
    });

    return (n > 0);
}


bool LauncherArgumentTokenizer::havePositionalArgs() const
{
    return haveArgsIn(positionalArgs);
}

const std::set<std::string> aircraftArgs({"state", "aircraft", "aircraft-dir", "vehicle"});

bool LauncherArgumentTokenizer::haveAircraftArgs() const
{
    return haveArgsIn(aircraftArgs);
}
