/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _REPLAY_VIEW
#define _REPLAY_VIEW

template <class T>
class ReplayView : public T {
  public:
    ReplayView(InputArchive& iff) : input(iff) {
    }

    virtual void close() override {
      T::close();
    }

    virtual void drawLevelMap(const CreatureView*) override {
      return;
    }

    bool equal(double a, double b) {
      return fabs(a - b) < 0.001;
    }

    virtual int getTimeMilli() override {
      string method;
      int time;
      input >> method >> time;
      CHECKEQ(method, "getTime");
      return time;
    }

    virtual void presentText(const string& title, const string& text) override {
      return;
    }

    virtual bool isClockStopped() override {
      T::isClockStopped();
      string method;
      bool ret;
      input >> method >> ret;
      CHECKEQ(method, "isClockStopped");
      return ret;
    }

    virtual UserInput getAction() override {
      T::getAction();
 //     usleep(300000);
      string method;
      UserInput action;
      input >> method >> action;
      if (method == "exit")
        exit(0);
      CHECKEQ(method, "getAction");
      return action;
    }

    virtual Optional<int> chooseFromList(const string& title, const vector<View::ListElem>& options, int index,
        View::MenuType, int* scrollPos, Optional<UserInputId> a) override {
      string method;
      Optional<int> action;
      input >> method >> action;
      CHECKEQ(method, "chooseFromList");
      return action;
    }

    virtual Optional<Vec2> chooseDirection(const string& message) override {
      string method;
      Optional<Vec2> action;
      input >> method >> action;
      CHECKEQ(method, "chooseDirection");
      return action;
    }

    virtual bool yesOrNoPrompt(const string& message) override {
      string method;
      bool action;
      input >> method >> action;
      CHECKEQ(method, "yesOrNoPrompt");
      return action;
    }

    virtual Optional<int> getNumber(const string& title, int min, int max, int increments) override {
      string method;
      Optional<int> action;
      input >> method >> action;
      CHECKEQ(method, "getNumber");
      return action;
    }
  private:
    InputArchive& input;
};

#endif
