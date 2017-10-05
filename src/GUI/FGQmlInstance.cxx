#include "FGQmlInstance.hxx"

#include <QDebug>
#include <QJSValueIterator>

#include "FGQmlPropertyNode.hxx"

#include <simgear/structure/commands.hxx>

#include <Main/fg_props.hxx>

namespace {

void convertJSObjectToPropertyNode(QJSValue js, SGPropertyNode *node);

void convertSingleJSValue(QJSValue v, SGPropertyNode* node)
{
    if (v.isNumber()) {
        node->setDoubleValue(v.toNumber());
    } else if (v.isBool()) {
        node->setBoolValue(v.toBool());
    } else if (v.isString()) {
        node->setStringValue(v.toString().toStdString());
    } else {
        qWarning() << Q_FUNC_INFO << "unhandld JS type";
    }
}

void convertJSObjectToPropertyNode(QJSValue js, SGPropertyNode* node)
{
    QJSValueIterator it(js);
    while (it.hasNext()) {
        it.next();
        const auto propName = it.name().toStdString();
        QJSValue v = it.value();
        if (v.isObject()) {
            // recursion is fun :)
            SGPropertyNode_ptr childNode = node->getChild(propName, true);
            convertJSObjectToPropertyNode(v, childNode);
        } else if (v.isArray()) {
            // mapping arbitrary JS arrays is slightly uncomfortable, but
            // this makes the common case work:
            //   foobar: [1,4, "apples", false]
            //   becomes foobar[0] = 1; foobar[1] = 4; foobar[2] = "apples";
            //   foobar[3] = false;
            // not perfect but useful enough to be worth supporting.
            QJSValueIterator arrayIt(v);
            int propIndex = 0;
            while (arrayIt.hasNext()) {
                arrayIt.next();
                SGPropertyNode_ptr childNode = node->getChild(propName, propIndex++, true);
                if (arrayIt.value().isArray()) {
                    // there is no sensible mapping of this to SGPropertyNode
                    qWarning() << Q_FUNC_INFO <<"arrays of array not possible";
                } else if (arrayIt.value().isObject()) {
                    convertJSObjectToPropertyNode(arrayIt.value(), childNode);
                } else {
                    // simple case, just convert the value in place
                    convertSingleJSValue(arrayIt.value(), childNode);
                }
            }
        } else {
            convertSingleJSValue(v, node->getChild(propName, true));
        }
    }
}

} // of anonymous namespace

FGQmlInstance::FGQmlInstance(QObject *parent) : QObject(parent)
{

}

bool FGQmlInstance::command(QString name, QJSValue args)
{
    const std::string cmdName = name.toStdString();

    SGCommandMgr* mgr = SGCommandMgr::instance();
    if (!mgr->getCommand(cmdName)) {
        qWarning() << "No such command" << name;
        return false;
    }

    SGPropertyNode_ptr propArgs(new SGPropertyNode);

    // convert JSValue args into a property tree.
    if (args.isObject()) {
        convertJSObjectToPropertyNode(args, propArgs);
    }

    //////

    return mgr->execute(cmdName, propArgs, globals->get_props());
}

FGQmlPropertyNode *FGQmlInstance::property(QString path, bool create) const
{
    SGPropertyNode_ptr node = fgGetNode(path.toStdString(), create);
    if (!node)
        return nullptr;

    FGQmlPropertyNode* result = new FGQmlPropertyNode;
    result->setNode(node);
    return result;
}
