
#ifndef DataclipP__H
#define DataclipP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_canvas.h>

#include <Xkw/Dataclip.h>

typedef struct _DataclipPart
{
    /*  Public resources  */
    iarray          array;
    XtCallbackList  regionCallback;
    int             maxRegions;
    int             pad;
    Bool            verbose;
    Bool            showBlankControl;
    Cardinal        fixedOutputType;
    /*  Private resources: must start on a  double  boundary  */
    double data_min;
    double data_max;
    double left_pos;
    double *minima;
    double *maxima;
    unsigned int num_regions;
    KPixCanvas pixcanvas;
    KWorldCanvas worldcanvas;
    KCallbackFunc iarr_destroy_callback;
    double *histogram_array;
    unsigned hist_arr_length;
    unsigned hist_buf_length;
    flag immediate_apply;
    flag last_was_left;
    flag blank_data;
    Widget min_label;
    Widget max_label;
    Widget lower_label;
    Widget upper_label;
    Widget dialog;
    Widget cnv;
    flag popped_up;
    unsigned int output_type;
} DataclipPart, *DataclipPartPtr;

typedef struct _DataclipRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    DataclipPart dataclip;
} DataclipRec, *DataclipPtr;

typedef struct _DataclipClassPart
{
    unsigned int type_index_to_type[NUMTYPES];
    char *type_names[NUMTYPES];
    unsigned int num_types;
} DataclipClassPart;

typedef struct _DataclipClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    DataclipClassPart dataclip_class;
} DataclipClassRec, *DataclipClassPtr;

extern DataclipClassRec dataclipClassRec;

#endif
