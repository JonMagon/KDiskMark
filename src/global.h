#ifndef GLOBAL_H
#define GLOBAL_H

class QString;

class Global
{
private:
    Global() {}
    ~Global() {}
    Global(const Global&);
    Global& operator=(const Global&);

public:
  static Global& Instance()
  {
    static Global singleton;
    return singleton;
  }

  QString getToolTipTemplate();
  QString getComparisonLabelTemplate();
  int getOutputColumnsCount();
};

#endif // GLOBAL_H
