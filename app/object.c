#include "object.h"
#include "objectF.h"
#include "objectP.h"

#if GTK_MAJOR_VERSION > 1
static void    gimp_object_dispose           (GObject         *object);
static void    gimp_object_finalize          (GObject         *object);
static void    gimp_object_set_property      (GObject         *object,
                                              guint            property_id,
                                              const GValue    *value,
                                              GParamSpec      *pspec);
static void    gimp_object_get_property      (GObject         *object,
                                              guint            property_id,
                                              GValue          *value,
                                              GParamSpec      *pspec);
static gint64  gimp_object_real_get_memsize  (GimpObject      *object,
                                              gint64          *gui_size);

enum
{
  DISCONNECT,
  NAME_CHANGED,
  LAST_SIGNAL
};
enum
{
  PROP_0,
  PROP_NAME
};

static GObjectClass *parent_class = NULL;
static guint   object_signals[LAST_SIGNAL] = { 0 };

#define gimp_marshal_VOID__VOID   g_cclosure_marshal_VOID__VOID
#endif

static void
gimp_object_init (GimpObject *object)
{
#if GTK_MAJOR_VERSION > 1
  object->name       = NULL;
  object->normalized = NULL;
#endif
}

static void
gimp_object_class_init (GimpObjectClass *klass)
{
#if GTK_MAJOR_VERSION > 1
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_signals[DISCONNECT] =
    g_signal_new ("disconnect",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpObjectClass, disconnect),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_signals[NAME_CHANGED] =
    g_signal_new ("name-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpObjectClass, name_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose      = gimp_object_dispose;
  object_class->finalize     = gimp_object_finalize;
  object_class->set_property = gimp_object_set_property;
  object_class->get_property = gimp_object_get_property;

  klass->disconnect          = NULL;
  klass->name_changed        = NULL;
  klass->get_memsize         = gimp_object_real_get_memsize;

  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE));
#endif
}

GtkType gimp_object_get_type (void)
{
	static GtkType type = 0;
#if GTK_MAJOR_VERSION > 1
  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpObjectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_object_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpObject),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_object_init,
      };

      type = g_type_register_static (GTK_TYPE_OBJECT,
                                            "GimpObject",
                                            &info, 0);
    }
#else
	GIMP_TYPE_INIT(type,
		       GimpObject,
		       GimpObjectClass,
		       gimp_object_init,
		       gimp_object_class_init,
		       GTK_TYPE_OBJECT);
#endif
	return type;
}




#if GTK_MAJOR_VERSION > 1
/* from core/gimp-utils.c */
gint64
gimp_g_object_get_memsize (GObject *object)
{
  GTypeQuery type_query;
  gint64     memsize = 0;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  g_type_query (G_TYPE_FROM_INSTANCE (object), &type_query);

  memsize += type_query.instance_size;

  return memsize;
}




/**
 * gimp_object_name_changed:
 * @object: a #GimpObject
 *
 * Causes the "name_changed" signal to be emitted.
 **/
void
gimp_object_name_changed (GimpObject *object)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  g_signal_emit (object, object_signals[NAME_CHANGED], 0);
}

/**
 * gimp_object_name_free:
 * @object: a #GimpObject
 *
 * Frees the name of @object and sets the name pointer to %NULL. Also
 * takes care of the normalized name that the object might be caching.
 *
 * In general you should be using gimp_object_set_name() instead. But
 * if you ever need to free the object name but don't want the
 * "name_changed" signal to be emitted, then use this function. Never
 * ever free the object name directly!
 **/
void
gimp_object_name_free (GimpObject *object)
{
  if (object->normalized)
    {
      if (object->normalized != object->name)
        g_free (object->normalized);

      object->normalized = NULL;
    }

  if (object->name)
    {
      g_free (object->name);
      object->name = NULL;
    }
}

/**
 * gimp_object_set_name:
 * @object: a #GimpObject
 * @name: the @object's new name
 *
 * Sets the @object's name. Takes care of freeing the old name and
 * emitting the "name_changed" signal if the old and new name differ.
 **/
void
gimp_object_set_name (GimpObject  *object,
                      const gchar *name)
{
  g_return_if_fail (GIMP_IS_OBJECT (object));

  if ((!object->name && !name) ||
      (object->name && name && !strcmp (object->name, name)))
    return;

  gimp_object_name_free (object);

  object->name = g_strdup (name);

  gimp_object_name_changed (object);
}



static void
gimp_object_dispose (GObject *object)
{
  gboolean disconnected;

  disconnected = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (object),
                                                     "disconnected"));

  if (! disconnected)
    {
      g_signal_emit (object, object_signals[DISCONNECT], 0);

      g_object_set_data (G_OBJECT (object), "disconnected",
                         GINT_TO_POINTER (TRUE));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_object_finalize (GObject *object)
{
  gimp_object_name_free (GIMP_OBJECT (object));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_object_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpObject *gimp_object = GIMP_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      gimp_object_set_name (gimp_object, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_object_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpObject *gimp_object = GIMP_OBJECT (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, gimp_object->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_object_real_get_memsize (GimpObject *object,
                              gint64     *gui_size)
{
  gint64  memsize = 0;

  if (object->name)
    memsize += strlen (object->name) + 1;

  return memsize + gimp_g_object_get_memsize ((GObject *) object);
}


#endif   									     
