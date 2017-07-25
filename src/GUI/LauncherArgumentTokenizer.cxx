#include "LauncherArgumentTokenizer.hxx"

LauncherArgumentTokenizer::LauncherArgumentTokenizer()
{

}

QList<LauncherArgumentTokenizer::Arg> LauncherArgumentTokenizer::tokenize(QString in) const
{
    int index = 0;
    const int len = in.count();
    QChar c, nc;
    State state = Start;
    QString key, value;
    QList<Arg> result;

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
                    return QList<Arg>();
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
                result.append(Arg(key));
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
                result.append(Arg(key, value));
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
        result.append(Arg(key));
    } else if (state == Value) {
        result.append(Arg(key, value));
    }

    return result;
}
