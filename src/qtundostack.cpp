/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qdebug.h>
#include "qtundostack.h"

QT_BEGIN_NAMESPACE

/*!
    \class QtUndoCommand
    \brief The QtUndoCommand class is the base class of all commands stored on a QtUndoStack.
    \since 4.2

    \inmodule QtWidgets

    For an overview of Qt's Undo Framework, see the
    \l{Overview of Qt's Undo Framework}{overview document}.

    A QtUndoCommand represents a single editing action on a document; for example,
    inserting or deleting a block of text in a text editor. QtUndoCommand can apply
    a change to the document with redo() and undo the change with undo(). The
    implementations for these functions must be provided in a derived class.

    \snippet code/src_gui_util_qundostack.cpp 0

    A QtUndoCommand has an associated text(). This is a short string
    describing what the command does. It is used to update the text
    properties of the stack's undo and redo actions; see
    QtUndoStack::createUndoAction() and QtUndoStack::createRedoAction().

    QtUndoCommand objects are owned by the stack they were pushed on.
    QtUndoStack deletes a command if it has been undone and a new command is pushed. For example:

\snippet code/src_gui_util_qundostack.cpp 1

    In effect, when a command is pushed, it becomes the top-most command
    on the stack.

    To support command compression, QtUndoCommand has an id() and the virtual function
    mergeWith(). These functions are used by QtUndoStack::push().

    To support command macros, a QtUndoCommand object can have any number of child
    commands. Undoing or redoing the parent command will cause the child
    commands to be undone or redone. A command can be assigned
    to a parent explicitly in the constructor. In this case, the command
    will be owned by the parent.

    The parent in this case is usually an empty command, in that it doesn't
    provide its own implementation of undo() and redo(). Instead, it uses
    the base implementations of these functions, which simply call undo() or
    redo() on all its children. The parent should, however, have a meaningful
    text().

    \snippet code/src_gui_util_qundostack.cpp 2

    Another way to create macros is to use the convenience functions
    QtUndoStack::beginMacro() and QtUndoStack::endMacro().

    \sa QtUndoStack
*/

class QtUndoCommandPrivate
{
public:
    QtUndoCommandPrivate() : id(-1) {}
    QList<QtUndoCommand*> child_list;
    QString text;
    QString actionText;
    int id;
    QtUndoCommand *q_ptr;
};

class QtUndoStackPrivate
{
public:
    QtUndoStackPrivate() : index(0), clean_index(0), undo_limit(0) {}

    QList<QtUndoCommand*> command_list;
    QList<QtUndoCommand*> macro_stack;
    int index;
    int clean_index;
    int undo_limit;

    void setIndex(int idx, bool clean);
    bool checkUndoLimit();

    QtUndoStack *q_ptr;
};

/*!
    Constructs a QtUndoCommand object with the given \a parent and \a text.

    If \a parent is not 0, this command is appended to parent's child list.
    The parent command then owns this command and will delete it in its
    destructor.

    \sa ~QtUndoCommand()
*/

QtUndoCommand::QtUndoCommand(const QString &text, QtUndoCommand *parent)
{
    d_ptr = new QtUndoCommandPrivate;
    d_ptr->q_ptr = this;

    if (parent != 0)
        parent->d_ptr->child_list.append(this);
    setText(text);
}

/*!
    Constructs a QtUndoCommand object with parent \a parent.

    If \a parent is not 0, this command is appended to parent's child list.
    The parent command then owns this command and will delete it in its
    destructor.

    \sa ~QtUndoCommand()
*/

QtUndoCommand::QtUndoCommand(QtUndoCommand *parent)
{
    d_ptr = new QtUndoCommandPrivate;
    d_ptr->q_ptr = this;

    if (parent != 0)
        parent->d_ptr->child_list.append(this);
}

/*!
    Destroys the QtUndoCommand object and all child commands.

    \sa QtUndoCommand()
*/

QtUndoCommand::~QtUndoCommand()
{
    qDeleteAll(d_ptr->child_list);
    delete d_ptr;
}

/*!
    Returns the ID of this command.

    A command ID is used in command compression. It must be an integer unique to
    this command's class, or -1 if the command doesn't support compression.

    If the command supports compression this function must be overridden in the
    derived class to return the correct ID. The base implementation returns -1.

    QtUndoStack::push() will only try to merge two commands if they have the
    same ID, and the ID is not -1.

    \sa mergeWith(), QtUndoStack::push()
*/

int QtUndoCommand::id() const
{
    return -1;
}

/*!
    Attempts to merge this command with \a command. Returns \c true on
    success; otherwise returns \c false.

    If this function returns \c true, calling this command's redo() must have the same
    effect as redoing both this command and \a command.
    Similarly, calling this command's undo() must have the same effect as undoing
    \a command and this command.

    QtUndoStack will only try to merge two commands if they have the same id, and
    the id is not -1.

    The default implementation returns \c false.

    \snippet code/src_gui_util_qundostack.cpp 3

    \sa id(), QtUndoStack::push()
*/

bool QtUndoCommand::mergeWith(const QtUndoCommand *command)
{
    Q_UNUSED(command);
    return false;
}

/*!
    Applies a change to the document. This function must be implemented in
    the derived class. Calling QtUndoStack::push(),
    QtUndoStack::undo() or QtUndoStack::redo() from this function leads to
    undefined beahavior.

    The default implementation calls redo() on all child commands.

    \sa undo()
*/

void QtUndoCommand::redo()
{
    for (int i = 0; i < d_ptr->child_list.size(); ++i)
        d_ptr->child_list.at(i)->redo();
}

/*!
    Reverts a change to the document. After undo() is called, the state of
    the document should be the same as before redo() was called. This function must
    be implemented in the derived class. Calling QtUndoStack::push(),
    QtUndoStack::undo() or QtUndoStack::redo() from this function leads to
    undefined beahavior.

    The default implementation calls undo() on all child commands in reverse order.

    \sa redo()
*/

void QtUndoCommand::undo()
{
    for (int i = d_ptr->child_list.size() - 1; i >= 0; --i)
        d_ptr->child_list.at(i)->undo();
}

/*!
    Returns a short text string describing what this command does; for example,
    "insert text".

    The text is used for names of items in QtUndoView.

    \sa actionText(), setText(), QtUndoStack::createUndoAction(), QtUndoStack::createRedoAction()
*/

QString QtUndoCommand::text() const
{
    return d_ptr->text;
}

/*!
    \since 4.8

    Returns a short text string describing what this command does; for example,
    "insert text".

    The text is used when the text properties of the stack's undo and redo
    actions are updated.

    \sa text(), setText(), QtUndoStack::createUndoAction(), QtUndoStack::createRedoAction()
*/

QString QtUndoCommand::actionText() const
{
    return d_ptr->actionText;
}

/*!
    Sets the command's text to be the \a text specified.

    The specified text should be a short user-readable string describing what this
    command does.

    If you need to have two different strings for text() and actionText(), separate
    them with "\\n" and pass into this function. Even if you do not use this feature
    for English strings during development, you can still let translators use two
    different strings in order to match specific languages' needs.
    The described feature and the function actionText() are available since Qt 4.8.

    \sa text(), actionText(), QtUndoStack::createUndoAction(), QtUndoStack::createRedoAction()
*/

void QtUndoCommand::setText(const QString &text)
{
    int cdpos = text.indexOf(QLatin1Char('\n'));
    if (cdpos > 0) {
        d_ptr->text = text.left(cdpos);
        d_ptr->actionText = text.mid(cdpos + 1);
    } else {
        d_ptr->text = text;
        d_ptr->actionText = text;
    }
}

/*!
    \since 4.4

    Returns the number of child commands in this command.

    \sa child()
*/

int QtUndoCommand::childCount() const
{
    return d_ptr->child_list.count();
}

/*!
    \since 4.4

    Returns the child command at \a index.

    \sa childCount(), QtUndoStack::command()
*/

const QtUndoCommand *QtUndoCommand::child(int index) const
{
    if (index < 0 || index >= d_ptr->child_list.count())
        return 0;
    return d_ptr->child_list.at(index);
}

/*!
    \class QtUndoStack
    \brief The QtUndoStack class is a stack of QtUndoCommand objects.
    \since 4.2

    \inmodule QtWidgets

    For an overview of Qt's Undo Framework, see the
    \l{Overview of Qt's Undo Framework}{overview document}.

    An undo stack maintains a stack of commands that have been applied to a
    document.

    New commands are pushed on the stack using push(). Commands can be
    undone and redone using undo() and redo(), or by triggering the
    actions returned by createUndoAction() and createRedoAction().

    QtUndoStack keeps track of the \a current command. This is the command
    which will be executed by the next call to redo(). The index of this
    command is returned by index(). The state of the edited object can be
    rolled forward or back using setIndex(). If the top-most command on the
    stack has already been redone, index() is equal to count().

    QtUndoStack provides support for undo and redo actions, command
    compression, command macros, and supports the concept of a
    \e{clean state}.

    \section1 Undo and Redo Actions

    QtUndoStack provides convenient undo and redo QAction objects, which
    can be inserted into a menu or a toolbar. When commands are undone or
    redone, QtUndoStack updates the text properties of these actions
    to reflect what change they will trigger. The actions are also disabled
    when no command is available for undo or redo. These actions
    are returned by QtUndoStack::createUndoAction() and QtUndoStack::createRedoAction().

    \section1 Command Compression and Macros

    Command compression is useful when several commands can be compressed
    into a single command that can be undone and redone in a single operation.
    For example, when a user types a character in a text editor, a new command
    is created. This command inserts the character into the document at the
    cursor position. However, it is more convenient for the user to be able
    to undo or redo typing of whole words, sentences, or paragraphs.
    Command compression allows these single-character commands to be merged
    into a single command which inserts or deletes sections of text.
    For more information, see QtUndoCommand::mergeWith() and push().

    A command macro is a sequence of commands, all of which are undone and
    redone in one go. Command macros are created by giving a command a list
    of child commands.
    Undoing or redoing the parent command will cause the child commands to
    be undone or redone. Command macros may be created explicitly
    by specifying a parent in the QtUndoCommand constructor, or by using the
    convenience functions beginMacro() and endMacro().

    Although command compression and macros appear to have the same effect to the
    user, they often have different uses in an application. Commands that
    perform small changes to a document may be usefully compressed if there is
    no need to individually record them, and if only larger changes are relevant
    to the user.
    However, for commands that need to be recorded individually, or those that
    cannot be compressed, it is useful to use macros to provide a more convenient
    user experience while maintaining a record of each command.

    \section1 Clean State

    QtUndoStack supports the concept of a clean state. When the
    document is saved to disk, the stack can be marked as clean using
    setClean(). Whenever the stack returns to this state through undoing and
    redoing commands, it emits the signal cleanChanged(). This signal
    is also emitted when the stack leaves the clean state. This signal is
    usually used to enable and disable the save actions in the application,
    and to update the document's title to reflect that it contains unsaved
    changes.

    \sa QtUndoCommand, QtUndoView
*/

/*! \internal
    Sets the current index to \a idx, emitting appropriate signals. If \a clean is true,
    makes \a idx the clean index as well.
*/

void QtUndoStackPrivate::setIndex(int idx, bool clean)
{
    bool was_clean = index == clean_index;

    if (idx != index) {
        index = idx;
        emit q_ptr->indexChanged(index);
        emit q_ptr->canUndoChanged(q_ptr->canUndo());
        emit q_ptr->undoTextChanged(q_ptr->undoText());
        emit q_ptr->canRedoChanged(q_ptr->canRedo());
        emit q_ptr->redoTextChanged(q_ptr->redoText());
    }

    if (clean)
        clean_index = index;

    bool is_clean = index == clean_index;
    if (is_clean != was_clean)
        emit q_ptr->cleanChanged(is_clean);
}

/*! \internal
    If the number of commands on the stack exceedes the undo limit, deletes commands from
    the bottom of the stack.

    Returns \c true if commands were deleted.
*/

bool QtUndoStackPrivate::checkUndoLimit()
{
    if (undo_limit <= 0 || !macro_stack.isEmpty() || undo_limit >= command_list.count())
        return false;

    int del_count = command_list.count() - undo_limit;

    for (int i = 0; i < del_count; ++i)
        delete command_list.takeFirst();

    index -= del_count;
    if (clean_index != -1) {
        if (clean_index < del_count)
            clean_index = -1; // we've deleted the clean command
        else
            clean_index -= del_count;
    }

    return true;
}

/*!
    Constructs an empty undo stack with the parent \a parent. The
    stack will initially be in the clean state. If \a parent is a
    QtUndoGroup object, the stack is automatically added to the group.

    \sa push()
*/

QtUndoStack::QtUndoStack(QObject *parent)
    : QObject(parent), d_ptr(new QtUndoStackPrivate)
{
  d_ptr->q_ptr = this;
}

/*!
    Destroys the undo stack, deleting any commands that are on it. If the
    stack is in a QtUndoGroup, the stack is automatically removed from the group.

    \sa QtUndoStack()
*/

QtUndoStack::~QtUndoStack()
{
    clear();
    delete d_ptr;
}

/*!
    Clears the command stack by deleting all commands on it, and returns the stack
    to the clean state.

    Commands are not undone or redone; the state of the edited object remains
    unchanged.

    This function is usually used when the contents of the document are
    abandoned.

    \sa QtUndoStack()
*/

void QtUndoStack::clear()
{
    if (d_ptr->command_list.isEmpty())
        return;

    bool was_clean = isClean();

    d_ptr->macro_stack.clear();
    qDeleteAll(d_ptr->command_list);
    d_ptr->command_list.clear();

    d_ptr->index = 0;
    d_ptr->clean_index = 0;

    emit indexChanged(0);
    emit canUndoChanged(false);
    emit undoTextChanged(QString());
    emit canRedoChanged(false);
    emit redoTextChanged(QString());

    if (!was_clean)
        emit cleanChanged(true);
}

/*!
    Pushes \a cmd on the stack or merges it with the most recently executed command.
    In either case, executes \a cmd by calling its redo() function.

    If \a cmd's id is not -1, and if the id is the same as that of the
    most recently executed command, QtUndoStack will attempt to merge the two
    commands by calling QtUndoCommand::mergeWith() on the most recently executed
    command. If QtUndoCommand::mergeWith() returns \c true, \a cmd is deleted.

    In all other cases \a cmd is simply pushed on the stack.

    If commands were undone before \a cmd was pushed, the current command and
    all commands above it are deleted. Hence \a cmd always ends up being the
    top-most on the stack.

    Once a command is pushed, the stack takes ownership of it. There
    are no getters to return the command, since modifying it after it has
    been executed will almost always lead to corruption of the document's
    state.

    \sa QtUndoCommand::id(), QtUndoCommand::mergeWith()
*/

void QtUndoStack::push(QtUndoCommand *cmd)
{
    cmd->redo();

    bool macro = !d_ptr->macro_stack.isEmpty();

    QtUndoCommand *cur = 0;
    if (macro) {
        QtUndoCommand *macro_cmd = d_ptr->macro_stack.last();
        if (!macro_cmd->d_ptr->child_list.isEmpty())
            cur = macro_cmd->d_ptr->child_list.last();
    } else {
        if (d_ptr->index > 0)
            cur = d_ptr->command_list.at(d_ptr->index - 1);
        while (d_ptr->index < d_ptr->command_list.size())
            delete d_ptr->command_list.takeLast();
        if (d_ptr->clean_index > d_ptr->index)
            d_ptr->clean_index = -1; // we've deleted the clean state
    }

    bool try_merge = cur != 0
                        && cur->id() != -1
                        && cur->id() == cmd->id()
                        && (macro || d_ptr->index != d_ptr->clean_index);

    if (try_merge && cur->mergeWith(cmd)) {
        delete cmd;
        if (!macro) {
            emit indexChanged(d_ptr->index);
            emit canUndoChanged(canUndo());
            emit undoTextChanged(undoText());
            emit canRedoChanged(canRedo());
            emit redoTextChanged(redoText());
        }
    } else {
        if (macro) {
            d_ptr->macro_stack.last()->d_ptr->child_list.append(cmd);
        } else {
            d_ptr->command_list.append(cmd);
            d_ptr->checkUndoLimit();
            d_ptr->setIndex(d_ptr->index + 1, false);
        }
    }
}

/*!
    Marks the stack as clean and emits cleanChanged() if the stack was
    not already clean.

    Whenever the stack returns to this state through the use of undo/redo
    commands, it emits the signal cleanChanged(). This signal is also
    emitted when the stack leaves the clean state.

    \sa isClean(), cleanIndex()
*/

void QtUndoStack::setClean()
{
    if (!d_ptr->macro_stack.isEmpty()) {
        qWarning("QtUndoStack::setClean(): cannot set clean in the middle of a macro");
        return;
    }

    d_ptr->setIndex(d_ptr->index, true);
}

/*!
    If the stack is in the clean state, returns \c true; otherwise returns \c false.

    \sa setClean(), cleanIndex()
*/

bool QtUndoStack::isClean() const
{
    if (!d_ptr->macro_stack.isEmpty())
        return false;
    return d_ptr->clean_index == d_ptr->index;
}

/*!
    Returns the clean index. This is the index at which setClean() was called.

    A stack may not have a clean index. This happens if a document is saved,
    some commands are undone, then a new command is pushed. Since
    push() deletes all the undone commands before pushing the new command, the stack
    can't return to the clean state again. In this case, this function returns -1.

    \sa isClean(), setClean()
*/

int QtUndoStack::cleanIndex() const
{
    return d_ptr->clean_index;
}

/*!
    Undoes the command below the current command by calling QtUndoCommand::undo().
    Decrements the current command index.

    If the stack is empty, or if the bottom command on the stack has already been
    undone, this function does nothing.

    \sa redo(), index()
*/

void QtUndoStack::undo()
{
    if (d_ptr->index == 0)
        return;

    if (!d_ptr->macro_stack.isEmpty()) {
        qWarning("QtUndoStack::undo(): cannot undo in the middle of a macro");
        return;
    }

    int idx = d_ptr->index - 1;
    d_ptr->command_list.at(idx)->undo();
    d_ptr->setIndex(idx, false);
}

/*!
    Redoes the current command by calling QtUndoCommand::redo(). Increments the current
    command index.

    If the stack is empty, or if the top command on the stack has already been
    redone, this function does nothing.

    \sa undo(), index()
*/

void QtUndoStack::redo()
{
    if (d_ptr->index == d_ptr->command_list.size())
        return;

    if (!d_ptr->macro_stack.isEmpty()) {
        qWarning("QtUndoStack::redo(): cannot redo in the middle of a macro");
        return;
    }

    d_ptr->command_list.at(d_ptr->index)->redo();
    d_ptr->setIndex(d_ptr->index + 1, false);
}

/*!
    Returns the number of commands on the stack. Macro commands are counted as
    one command.

    \sa index(), setIndex(), command()
*/

int QtUndoStack::count() const
{
    return d_ptr->command_list.size();
}

/*!
    Returns the index of the current command. This is the command that will be
    executed on the next call to redo(). It is not always the top-most command
    on the stack, since a number of commands may have been undone.

    \sa undo(), redo(), count()
*/

int QtUndoStack::index() const
{
    return d_ptr->index;
}

/*!
    Repeatedly calls undo() or redo() until the current command index reaches
    \a idx. This function can be used to roll the state of the document forwards
    of backwards. indexChanged() is emitted only once.

    \sa index(), count(), undo(), redo()
*/

void QtUndoStack::setIndex(int idx)
{
    if (!d_ptr->macro_stack.isEmpty()) {
        qWarning("QtUndoStack::setIndex(): cannot set index in the middle of a macro");
        return;
    }

    if (idx < 0)
        idx = 0;
    else if (idx > d_ptr->command_list.size())
        idx = d_ptr->command_list.size();

    int i = d_ptr->index;
    while (i < idx)
        d_ptr->command_list.at(i++)->redo();
    while (i > idx)
        d_ptr->command_list.at(--i)->undo();

    d_ptr->setIndex(idx, false);
}

/*!
    Returns \c true if there is a command available for undo; otherwise returns \c false.

    This function returns \c false if the stack is empty, or if the bottom command
    on the stack has already been undone.

    Synonymous with index() == 0.

    \sa index(), canRedo()
*/

bool QtUndoStack::canUndo() const
{
    if (!d_ptr->macro_stack.isEmpty())
        return false;
    return d_ptr->index > 0;
}

/*!
    Returns \c true if there is a command available for redo; otherwise returns \c false.

    This function returns \c false if the stack is empty or if the top command
    on the stack has already been redone.

    Synonymous with index() == count().

    \sa index(), canUndo()
*/

bool QtUndoStack::canRedo() const
{
    if (!d_ptr->macro_stack.isEmpty())
        return false;
    return d_ptr->index < d_ptr->command_list.size();
}

/*!
    Returns the text of the command which will be undone in the next call to undo().

    \sa QtUndoCommand::actionText(), redoText()
*/

QString QtUndoStack::undoText() const
{
    if (!d_ptr->macro_stack.isEmpty())
        return QString();
    if (d_ptr->index > 0)
        return d_ptr->command_list.at(d_ptr->index - 1)->actionText();
    return QString();
}

/*!
    Returns the text of the command which will be redone in the next call to redo().

    \sa QtUndoCommand::actionText(), undoText()
*/

QString QtUndoStack::redoText() const
{
    if (!d_ptr->macro_stack.isEmpty())
        return QString();
    if (d_ptr->index < d_ptr->command_list.size())
        return d_ptr->command_list.at(d_ptr->index)->actionText();
    return QString();
}

/*!
    Begins composition of a macro command with the given \a text description.

    An empty command described by the specified \a text is pushed on the stack.
    Any subsequent commands pushed on the stack will be appended to the empty
    command's children until endMacro() is called.

    Calls to beginMacro() and endMacro() may be nested, but every call to
    beginMacro() must have a matching call to endMacro().

    While a macro is composed, the stack is disabled. This means that:
    \list
    \li indexChanged() and cleanChanged() are not emitted,
    \li canUndo() and canRedo() return false,
    \li calling undo() or redo() has no effect,
    \li the undo/redo actions are disabled.
    \endlist

    The stack becomes enabled and appropriate signals are emitted when endMacro()
    is called for the outermost macro.

    \snippet code/src_gui_util_qundostack.cpp 4

    This code is equivalent to:

    \snippet code/src_gui_util_qundostack.cpp 5

    \sa endMacro()
*/

void QtUndoStack::beginMacro(const QString &text)
{
    QtUndoCommand *cmd = new QtUndoCommand();
    cmd->setText(text);

    if (d_ptr->macro_stack.isEmpty()) {
        while (d_ptr->index < d_ptr->command_list.size())
            delete d_ptr->command_list.takeLast();
        if (d_ptr->clean_index > d_ptr->index)
            d_ptr->clean_index = -1; // we've deleted the clean state
        d_ptr->command_list.append(cmd);
    } else {
        d_ptr->macro_stack.last()->d_ptr->child_list.append(cmd);
    }
    d_ptr->macro_stack.append(cmd);

    if (d_ptr->macro_stack.count() == 1) {
        emit canUndoChanged(false);
        emit undoTextChanged(QString());
        emit canRedoChanged(false);
        emit redoTextChanged(QString());
    }
}

/*!
    Ends composition of a macro command.

    If this is the outermost macro in a set nested macros, this function emits
    indexChanged() once for the entire macro command.

    \sa beginMacro()
*/

void QtUndoStack::endMacro()
{
    if (d_ptr->macro_stack.isEmpty()) {
        qWarning("QtUndoStack::endMacro(): no matching beginMacro()");
        return;
    }

    d_ptr->macro_stack.removeLast();

    if (d_ptr->macro_stack.isEmpty()) {
        d_ptr->checkUndoLimit();
        d_ptr->setIndex(d_ptr->index + 1, false);
    }
}

/*!
  \since 4.4

  Returns a const pointer to the command at \a index.

  This function returns a const pointer, because modifying a command,
  once it has been pushed onto the stack and executed, almost always
  causes corruption of the state of the document, if the command is
  later undone or redone.

  \sa QtUndoCommand::child()
*/
const QtUndoCommand *QtUndoStack::command(int index) const
{
    if (index < 0 || index >= d_ptr->command_list.count())
        return 0;
    return d_ptr->command_list.at(index);
}

/*!
    Returns the text of the command at index \a idx.

    \sa beginMacro()
*/

QString QtUndoStack::text(int idx) const
{
    if (idx < 0 || idx >= d_ptr->command_list.size())
        return QString();
    return d_ptr->command_list.at(idx)->text();
}

/*!
    \property QtUndoStack::undoLimit
    \brief the maximum number of commands on this stack.
    \since 4.3

    When the number of commands on a stack exceedes the stack's undoLimit, commands are
    deleted from the bottom of the stack. Macro commands (commands with child commands)
    are treated as one command. The default value is 0, which means that there is no
    limit.

    This property may only be set when the undo stack is empty, since setting it on a
    non-empty stack might delete the command at the current index. Calling setUndoLimit()
    on a non-empty stack prints a warning and does nothing.
*/

void QtUndoStack::setUndoLimit(int limit)
{
    if (!d_ptr->command_list.isEmpty()) {
        qWarning("QtUndoStack::setUndoLimit(): an undo limit can only be set when the stack is empty");
        return;
    }

    if (limit == d_ptr->undo_limit)
        return;
    d_ptr->undo_limit = limit;
    d_ptr->checkUndoLimit();
}

int QtUndoStack::undoLimit() const
{
    return d_ptr->undo_limit;
}

/*!
    \fn void QtUndoStack::indexChanged(int idx)

    This signal is emitted whenever a command modifies the state of the document.
    This happens when a command is undone or redone. When a macro
    command is undone or redone, or setIndex() is called, this signal
    is emitted only once.

    \a idx specifies the index of the current command, ie. the command which will be
    executed on the next call to redo().

    \sa index(), setIndex()
*/

/*!
    \fn void QtUndoStack::cleanChanged(bool clean)

    This signal is emitted whenever the stack enters or leaves the clean state.
    If \a clean is true, the stack is in a clean state; otherwise this signal
    indicates that it has left the clean state.

    \sa isClean(), setClean()
*/

/*!
    \fn void QtUndoStack::undoTextChanged(const QString &undoText)

    This signal is emitted whenever the value of undoText() changes. It is
    used to update the text property of the undo action returned by createUndoAction().
    \a undoText specifies the new text.
*/

/*!
    \fn void QtUndoStack::canUndoChanged(bool canUndo)

    This signal is emitted whenever the value of canUndo() changes. It is
    used to enable or disable the undo action returned by createUndoAction().
    \a canUndo specifies the new value.
*/

/*!
    \fn void QtUndoStack::redoTextChanged(const QString &redoText)

    This signal is emitted whenever the value of redoText() changes. It is
    used to update the text property of the redo action returned by createRedoAction().
    \a redoText specifies the new text.
*/

/*!
    \fn void QtUndoStack::canRedoChanged(bool canRedo)

    This signal is emitted whenever the value of canRedo() changes. It is
    used to enable or disable the redo action returned by createRedoAction().
    \a canRedo specifies the new value.
*/

QT_END_NAMESPACE

