/* Copyright 2015 Ruan Luies */

#include <algorithm>
#include <QDir>
#include "./renderstate.h"
#include "Functions/mathematics.h"

RenderState::RenderState(QWidget *parent): QOpenGLWidget(parent),
    program(0),
    mouse_x(0),
    mouse_y(0),
    node_index_selected(-1),
    selected_floor_plan(-1),
    mouse_zoom(60.0f),
    noderadius(0.5f),
    mouse_y_inverted(1.0f),
    current_floor_height(0.0f),
    position_camera(QVector3D()),
    camera_prev(QVector3D()),
    raycast(QVector3D()),
    rotation(QVector3D()),
    currentscale(QVector3D(1, 1, 1)),
    drag_middle_position(QVector3D()),
    corner_1(QVector3D()),
    corner_2(QVector3D()),
    corner_3(QVector3D()),
    corner_4(QVector3D()),
    center_h_1(QVector3D()),
    center_h_2(QVector3D()),
    mousedown_right(false),
    mousedown_left(false),
    node_placable(false),
    pavement_placable(false),
    tree_placable(false),
    placable_floor_plan(false),
    node_significant(true),
    start_up_load_tex(true),
    edit_floorplan(false),
    edit_node(false),
    node_walk(false),
    node_wheelchair(false),
    node_vehicle(false),
    node_bicycle(false),
    tree_radius(4.0f),
    infinte_lenght_lines(100.0f),
    handler(){
    this->door_placeable = false;
    this->wall_placable = false;
    this->floor_plan_removable = false;
    this->link_removable = false;
    this->node_removable = false;
    // enable antialiasing (set the format of the widget)
    QSurfaceFormat format;
    format.setSamples(4);
    this->setFormat(format);

    // initialize the clicked position
    this->clicked_position = new QVector3D(-4445.3,-3455,-345555);

    // set the current mouse position in 3D
    this->current_position = new QVector3D(0,-2033,-93345);

    // clear the textures
    this->textures.clear();

    // set the position to initial null vector
    this->position = new QVector3D();

    // clear the nodes
    this->nodes.clear();

    // clear visual objects
    this->models.clear();

    // set mouse tracking
    setMouseTracking(true);

    // create directory
    PremisesExporter::create_director();

    // initialise the session
    session_logged = new QByteArray("");

    // initialized successfully
    emit debug_results("Premises Visualizer Initialized");

}

void RenderState::receive_session(QByteArray session) {
  *session_logged = session;
}

void RenderState::allow_edit_floor(bool allow) {
  edit_floorplan = allow;
  // display debugging message
  /*QString floor_plan = "Edit Floor Plans: ";
  if ( allow )
    floor_plan += "true";
  else
    floor_plan += "false";
  emit debug_results(floor_plan);*/
}

void RenderState::allow_edit_node(bool allow) {
  edit_node = allow;
}

void RenderState::allow_node(bool value) {
  this->node_placable = value;
  if ( value )
    this->node_index_selected = -1;
}

void RenderState::allow_remove_link(bool allow) {
  link_removable = allow;
}

void RenderState::invert_mouseY(bool value) {
  if (value)
    this->mouse_y_inverted = -1.0f;
  else
    this->mouse_y_inverted = 1.0f;
}

void RenderState::allow_remove_floor_plan(bool allow) {
    floor_plan_removable = allow;
}

void RenderState::load_premises(QString value) {
  // get the path of each subdirectory
  QStringList ls = value.split("/");

  // count the subdirectories
  int append = ls.count();
  QString directory_path = "";

  // create new file path from previous
  for ( int k = 0; k < append-1; k++ ) {
    directory_path += ls[k] + "/";
  }
  // load textures and objects
  LoadNodes(directory_path);
  LoadTextures(directory_path);
  LoadObjects(directory_path);
  CopyDirectories(directory_path + "/directories.dir");
  CopyConfig(directory_path + "/config.config");
  // exort the files afterwards
  PremisesExporter::export_environment(this->models,
                                       "environment.env");
  PremisesExporter::export_texture(this->texture_paths,
                                   "textures.tl");
  PremisesExporter::export_nodes(this->nodes,
                                 "nodes.pvc");
  // set startup load
  start_up_load_tex = false;
}

void RenderState::set_next_node_name(QString value) {
  this->next_node_name = value;
  if ( this->node_index_selected < this->nodes.count() &&
       this->node_index_selected > -1 )
    nodes.value(this->node_index_selected)->setName(value);
  // exort nodes
  PremisesExporter::export_nodes(this->nodes,
                                 "nodes.pvc");
}

void RenderState::set_next_node_significant(bool value) {
  this->node_significant = value;
  if ( this->node_index_selected > -1 &&
       this->edit_node &&
       this->node_index_selected < this->nodes.count() )
    nodes.value(this->node_index_selected)->setSignificant(value);
  // exort nodes
  PremisesExporter::export_nodes(this->nodes,
                                 "nodes.pvc");
}

void RenderState::allow_remove_node(bool value) {
  this->node_removable = value;
}

void RenderState::allow_remove_tree(bool value) {
  this->tree_removable = value;
}

void RenderState::allow_link(bool value) {
  this->node_linkable = value;
}

void RenderState::allow_pavement(bool value) {
  this->pavement_placable = value;
}

void RenderState::allow_door(bool value) {
  this->door_placeable = value;
}

void RenderState::allow_wall(bool value) {
  this->wall_placable = value;
}

void RenderState::allow_tree(bool value) {
  this->tree_placable = value;
}

void RenderState::allow_floor_plan(bool value) {
  this->placable_floor_plan = value;
}

void RenderState::change_rotY(double value) {
  this->rotation.setY(value);
  if ( this->edit_floorplan &&
       this->selected_floor_plan < this->models.count() &&
       this->selected_floor_plan > -1 )
      this->models.value(this->selected_floor_plan)->setRotation(
              QVector3D(0, value, 0));
  PremisesExporter::export_environment(this->models,
                                         "environment.env");

}

void RenderState::set_object_scale(QVector3D value) {
    this->currentscale = value;
    if ( this->edit_floorplan &&
         this->selected_floor_plan < this->models.count() &&
         this->selected_floor_plan > -1)
      this->models.value(this->selected_floor_plan)->setScaling(value);
    PremisesExporter::export_environment(this->models,
                                         "environment.env");
}

void RenderState::change_current_floor_height(float value) {
    this->current_floor_height = value;
    // update the opengl widget
    update();
}

void RenderState::load_texture_from_file(QString value) {
  QString val_new = "VirtualConcierge/" +
          QString("TEX") +
          QString::number(this->texture_paths.count());
  QDir dir;
  // try to copy the texture to the drive
  if ( QString::compare(dir.absolutePath() + "/" + val_new, value, Qt::CaseInsensitive) != 0 ) {
    if ( QFile::exists(val_new) && !start_up_load_tex ) {
     if ( !QFile::remove(val_new) ) {
        // QMessageBox::warning(this,
        //                      tr("Error file deleting"),
        //                      tr("Texture file could not"
        //                        " be deleted from the drive."));
     }
    }
    if ( !QFile::copy(value, val_new) ) {
      if ( !QFile::exists(val_new) ) {
        QMessageBox::warning(this,
                             tr("Error file copying"),
                             tr("Texture file could not"
                                " be copied to the drive."));
      }
    }
  }

  // add texture to the lists
  QOpenGLTexture* texture= new QOpenGLTexture(QImage(value).mirrored());
  texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
  texture->setMagnificationFilter(QOpenGLTexture::Linear);

  this->textures_from_files.push_back(texture);
  this->texture_paths.push_back(val_new);

}

void RenderState::initializeGL() {
  initializeOpenGLFunctions();
  // texture test
  for ( int i = 0; i < 1; i++ ) {
  QOpenGLTexture *texture =
          new QOpenGLTexture(QImage("://Texture" +
                                    QString::number(i)).mirrored());
  texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
  texture->setMagnificationFilter(QOpenGLTexture::Linear);
  this->textures.push_back(texture);
  }
}

void RenderState::mouseMoveEvent(QMouseEvent* event) {
  // alert mouse event's position (x)
  this->mouse_x = event->x();

  // alert mouse event's position (x)
  this->mouse_y = event->y();

  // update raycast vector
  this->raycast = Mathematics::mouse_raycast(this->mouse_x,
                                               this->mouse_y,
                                               this->width(),
                                               this->height(),
                                               this->mouse_y_inverted,
                                               this->vMatrix, pMatrix);

  if ( this->mousedown_right ) {
        this->position_camera.setX(this->clicked_position->x() -
                                   this->current_position->x());
        this->position_camera.setY(this->clicked_position->y() -
                                   this->current_position->y());
        this->position_camera.setZ(this->clicked_position->z() -
                                   this->current_position->z());

        // pan view
        this->camera_prev.setX(this->camera_prev.x() -
                               this->position_camera.x() * 0.5);
        this->camera_prev.setY(this->camera_prev.y() -
                               this->position_camera.y() * 0.5);
        this->camera_prev.setZ(this->camera_prev.z() -
                               this->position_camera.z() * 0.5);
        this->position_camera = QVector3D(0, 0, 0);
    }

  // removable dragable nodes
  if ( this->mousedown_left && this->node_removable ) {
    RemoveNodes();
  }

  // update openGL widget
  update();
}

void RenderState::mouseReleaseEvent(QMouseEvent * /*event*/) {
    // release button right click
    if ( this->mousedown_right )
    this->mousedown_right = false;

    // button left click released
    if ( this->mousedown_left ) {
        this->mousedown_left = false;

        if ( this->node_linkable ) {
            int linkindex = -1;
            // get position of the clicked
            this->clicked_position = new QVector3D(this->current_position->x(),
                                                   this->current_position->y(),
                                                   this->current_position->z());

            // collision detection
            for ( int l = 0; l < this->nodes.count(); l++ ) {
                if ( this->clicked_position->
                     distanceToPoint(this->nodes.value(l)->Position()) <
                     this->noderadius )
                    linkindex = l;
            }
            if ( (linkindex > -1) &&
                 (this->node_index_selected > -1) &&
                 (this->node_index_selected < this->nodes.count()) ) {
                const unsigned int countConnected =
                        this->
                        nodes.value(this->node_index_selected)->
                        countConnected();
                // add a link to the node
                this->nodes.value(this->node_index_selected)->
                        AddLink(new QString("Link" +
                                            QString::number(countConnected)),
                                linkindex);
            }
            // export to temp nodes
            PremisesExporter::export_nodes(this->nodes,
                                           "nodes.pvc");

            // update errors
            update_node_errors();
        }

        if ( this->wall_placable ) {
            // place wall
             add_wall(this->rotation,
                      this->drag_middle_position,
                      QVector3D(1, 1, this->currentscale.z()));
        }

        if ( this->pavement_placable ) {
            // place pavement
            add_pavement(this->rotation,
                         *this->clicked_position,
                         this->currentscale);
        }
    }

    this->clicked_position = new QVector3D(0, -1000, 0);
    this->rotation = QVector3D();
    // this->currentscale = QVector3D(1, 1, 1);
    // update the frame
    update();
}

void RenderState::edit_node_position(QVector2D position) {
    if ( this->node_index_selected > -1 &&
         this->edit_node &&
         this->node_index_selected < this->nodes.count() )
      nodes.value(this->node_index_selected)->setPosition(
                  QVector3D(position.x(),
                            nodes.value(this->node_index_selected)->Position().y(),
                            position.y()));
    // exort nodes
    PremisesExporter::export_nodes(this->nodes,
                                   "nodes.pvc");


}

void RenderState::edit_node_access(bool walk, bool wheelchair, bool vehicle, bool bicycle) {
    this->node_walk = walk;
    this->node_wheelchair = wheelchair;
    this->node_vehicle = vehicle;
    this->node_bicycle = bicycle;
    if ( this->node_index_selected > -1 &&
         this->edit_node &&
         this->node_index_selected < this->nodes.count() ) {
      nodes.value(this->node_index_selected)->setWalk(walk);
      nodes.value(this->node_index_selected)->setBike(bicycle);
      nodes.value(this->node_index_selected)->setWheelChair(wheelchair);
      nodes.value(this->node_index_selected)->setVehicle(vehicle);
    }
    // exort nodes
    PremisesExporter::export_nodes(this->nodes,
                                   "nodes.pvc");


    //LoadNodes("VirtualConcierge/");
    // update errors
    update_node_errors();
}

void RenderState::edit_floorplan_position(QVector2D position) {
    if ( this->edit_floorplan &&
         this->selected_floor_plan < this->models.count() &&
         this->selected_floor_plan > -1)
      this->models.value(this->selected_floor_plan)->setTranslation(
      QVector3D(position.x(),
                models.value(this->selected_floor_plan)->getTranslation().y(),
                position.y()));
    PremisesExporter::export_environment(this->models,
                                         "environment.env");

}

void RenderState::mousePressEvent(QMouseEvent* event) {
    // make dragable from left click
    if ( event->button() == Qt::LeftButton)
    this->mousedown_left = true;

    // right click to move the camara around
    if ( event->button() == Qt::RightButton) {
      this->mousedown_right = true;
        // if ( !this->edit_node )
          // this->node_index_selected = -1;
    }

    // left click to add the node
    if ( (event->button() == Qt::LeftButton) && (this->node_placable) ) {
        add_node(new QString(this->next_node_name));
        PremisesExporter::export_nodes(this->nodes, "nodes.pvc");

    }

    // left click to add door
    if ( (event->button() == Qt::LeftButton) && (this->door_placeable) )
        add_door(this->rotation, *this->current_position, this->currentscale);

    // add floor plan
    if ( (event->button() == Qt::LeftButton) && (this->placable_floor_plan) )
        add_floor_plan(this->rotation,
                       *this->current_position,
                       this->currentscale);

    // left click to add wall
    if ( (event->button() == Qt::LeftButton) && (this->wall_placable) )
        this->clicked_position = new QVector3D(this->current_position->x(),
                                               this->current_position->y(),
                                               this->current_position->z());

    // left click to add tree
    if ( (event->button() == Qt::LeftButton) && (this->tree_placable) )
        add_tree(this->rotation, *this->current_position, this->currentscale);

    // set current clicked position
    this->clicked_position = new QVector3D(this->current_position->x(),
                                           this->current_position->y(),
                                           this->current_position->z());

    if ( (event->button() == Qt::LeftButton) && (this->tree_removable) ) {
        for ( int k = 0 ; k < this->models.count(); k++ ) {
            if ( (this->models.value(k)->
                  getType().compare("Tree",
                                    Qt::CaseInsensitive) == 0) &&
                 (this->models.value(k)->
                  getTranslation().distanceToPoint(*this->current_position) <
                  tree_radius) ) {
                this->models.removeAt(k);
            }
        }
    }

  // left click to remove the node
  if ( (event->button() == Qt::LeftButton) && (this->node_removable) ) {
    RemoveNodes();
  }

  // left click to remove the node
  if ( (event->button() == Qt::LeftButton) && (this->edit_floorplan) ) {
    remove_select_floorplan();
  }

  // left click to remove the node
  if ( (event->button() == Qt::LeftButton) && (this->floor_plan_removable) ) {
    remove_select_floorplan();
  }

  // left click to remove the link
  if ( (event->button() == Qt::LeftButton) && (this->link_removable) ) {
    remove_link();
  }

  // left click to add the link
  if ( (event->button() == Qt::LeftButton) && (this->node_linkable) ) {
    // get position of the clicked
    this->clicked_position = new QVector3D(this->current_position->x(),
                                           this->current_position->y(),
                                           this->current_position->z());

    // collision detection
    for ( int l = 0; l < this->nodes.count(); l++ ) {
      if ( this->clicked_position->
           distanceToPoint(this->nodes.value(l)->Position()) < this->noderadius)
        this->node_index_selected = l;
    }
  }

  // left click to edit the node
  if ( (event->button() == Qt::LeftButton) && (this->edit_node) ) {
    // get position of the clicked
    this->clicked_position = new QVector3D(this->current_position->x(),
                                             this->current_position->y(),
                                             this->current_position->z());
    // collision detection
    for ( int l = 0; l < this->nodes.count(); l++ ) {
      if ( this->clicked_position->
        distanceToPoint(this->nodes.value(l)->Position()) < this->noderadius) {
        this->node_index_selected = l;
        send_edit_node(this->nodes.value(l)->getName(),
                       QVector2D(this->nodes.value(l)->Position().x(),
                                 this->nodes.value(l)->Position().z()),
                       this->nodes.value(l)->getSignificant(),
                       this->nodes.value(l)->getWalk(),
                       this->nodes.value(l)->getWheelChair(),
                       this->nodes.value(l)->getBike(),
                       this->nodes.value(l)->getVehicle());
        emit debug_results("Selected node index:" + QString::number(l));
      }
    }
  }
}

void RenderState::remove_link() {
  // remove link
  for ( int k = 0; k < this->nodes.count(); k++ ) {
    const unsigned int count_connected =
            this->nodes.value(k)->countConnected();
    QVector3D node_position_current = this->nodes.value(k)->Position();
    // remove all the links of the deleted node
    for ( unsigned int z = 0; z < count_connected; z++ ) {
      int connected_index = this->nodes.value(k)->getConnectedIndex(z);
      QVector3D alt_node_position =
              this->nodes.value(connected_index)->Position();
      if ( Mathematics::detect_point_near_line(node_position_current,
                                  alt_node_position,
                                  QVector3D(this->current_position->x(),
                                            this->current_position->y(),
                                            this->current_position->z()),
                                  0.25) ) {
        this->nodes.value(k)->RemoveLinkedFromIndex(z);
      }
    }
  }
  // update errors
  update_node_errors();
  // update the working files
  PremisesExporter::export_nodes(this->nodes, "nodes.pvc");

}

void RenderState::remove_select_floorplan() {
  /* floor plans are 2d rectangulars
   * that can be rotated only about the y-axis.
   * thus the rotation along with the scale of
   * the floor plan can be used to detect
   * the intersection of a point or a ray
   */
  for ( int l = 0; l < this->models.count(); l++ ) {
      if ( Mathematics::detect_point_in_plan_on_y(
               this->models.value(l)->getTranslation(),
               this->models.value(l)->getScaling(),
               this->models.value(l)->getRotation().y(),
               QVector3D(this->current_position->x(),
                         this->current_position->y(),
                         this->current_position->z()))) {
          if ( this->models.value(l)->getType() == "FloorPlan") {
              if ( this->floor_plan_removable ) {
                this->models.removeAt(l);
              } else {
                  if ( this->edit_floorplan ) {
                    this->selected_floor_plan = l;
                    if ( this->selected_floor_plan != -1 &&
                      this->selected_floor_plan < this->models.count() ) {
                      // get the translation of the
                      QVector2D pos_floor =
                      QVector2D(this->models.value(this->selected_floor_plan)->
                                getTranslation().x(),
                                this->models.value(this->selected_floor_plan)->
                                getTranslation().z());
                      QVector2D scale_floor =
                      QVector2D(this-> models.value(this->selected_floor_plan)->
                                getScaling().x(),
                                this->models.value(this->selected_floor_plan)->
                                getScaling().z());
                          emit send_edit_floorplan(
                                  pos_floor,
                                  this->models.value(this->selected_floor_plan)->
                                  getRotation().y(),
                                  scale_floor);
                    }
                  }
              }
          }
      }
  }
  PremisesExporter::export_environment(this->models, "environment.env");
  PremisesExporter::export_texture(this->texture_paths, "textures.tl");

}

void RenderState::RemoveNodes() {
  for ( int l = 0; l < this->nodes.count(); l++ ) {
    if ( this->current_position->distanceToPoint(this->
                                               nodes.value(l)->
                                               Position()) <
      this->noderadius ) {
      // remove node
      this->nodes.removeAt(l);

      // remove all dependencies
      for ( int i = 0; i < this->nodes.count(); i++ ) {
        const unsigned int count_connected =
                this->nodes.value(i)->countConnected();
        // remove all the links of the deleted node
        for ( unsigned int z = 0; z < count_connected; z++ ) {
          if ( this->nodes.value(i)->getConnectedIndex(z) == l)
            this->nodes.value(i)->RemoveLinkedFromIndex(z);
        }

        // move the links back after the node was deleted
        for ( unsigned int k = 0; k < count_connected; k++ ) {
          if ( this->nodes.value(i)->getConnectedIndex(k) > l )
            this->nodes.value(i)->MoveLinkedIndexBack(k);
        }
      }
    }
  }

  // update the temp nodelist
  PremisesExporter::export_nodes(this->nodes, "nodes.pvc");
  //handler.AddNodes(this->nodes);

  // show node errors
  update_node_errors();
}

void RenderState::wheelEvent(QWheelEvent* event) {
    // camera zoom with the mouse scroll
    this->mouse_zoom -= static_cast<float>(event->delta()) / 120.0f;

    // limit the zoom
    if ( this->mouse_zoom < 2.0f)
        this->mouse_zoom = 2.0f;

    // update the openGL frame after the zoom
    update();
}

void RenderState::update_node_errors() {
    // add nodes for debugging
    error_nodes.clear();
    handler.AddNodes(this->nodes);
    // displayed debugged nodes
    if ( handler.count() > 0 ) {
      QString error_msg = handler.DisplayError();
      if ( !error_msg.isEmpty() && (error_msg[0] != ' ') ) {
      emit debug_results(error_msg);
      error_nodes = handler.error_nodes_indices();
      }
    }
    for ( int i = 0; i < error_nodes.count(); i++ ) {
        if (error_nodes.value(i) < 0 && error_nodes.value(i) > this->nodes.count()) {
            error_nodes.remove(i);
        }
    }
}

void RenderState::add_node(QString* name) {
    // create new nodes
    Node *newnode = new Node(new QVector3D(this->current_position->x(),
                                           this->current_position->y(),
                                           this->current_position->z()),
                                           name);
    // set significance
    newnode->setSignificant(this->node_significant);

    // accessibility
    newnode->setWalk(this->node_walk);
    newnode->setWheelChair(this->node_wheelchair);
    newnode->setVehicle(this->node_vehicle);
    newnode->setBike(this->node_bicycle);

    // add new node to vector
    this->nodes.push_back(newnode);

    // show node errors
    update_node_errors();

    // update the working files
    PremisesExporter::export_nodes(this->nodes, "nodes.pvc");


}

void RenderState::add_pavement(QVector3D rotation,
                               QVector3D translation,
                               QVector3D scaling) {
    // texture index 1 is the tile
    VisualObject * object = new VisualObject(this->plane,
                                             this->textures.value(1),
                                             translation,
                                             rotation,
                                             "Pavement");
    object->setScaling(scaling);
    this->models.push_back(object);
    PremisesExporter::export_environment(this->models, "environment.env");
    PremisesExporter::export_texture(this->texture_paths, "textures.tl");
    object->setTextureID(701);
    object->setTexturePath("://Texture1");

}

void RenderState::add_tree(QVector3D rotation,
                           QVector3D translation,
                           QVector3D scaling) {
    // texture index 1 is the tile
    VisualObject * object = new VisualObject(this->tree,
                                             this->textures.value(2),
                                             translation,
                                             rotation,
                                             "Tree");
    object->setScaling(scaling);
    this->models.push_back(object);
    PremisesExporter::export_environment(this->models, "environment.env");
    PremisesExporter::export_texture(this->texture_paths, "textures.tl");
    object->setTextureID(702);
    object->setTexturePath("://Texture2");

}

void RenderState::add_wall(QVector3D rotation,
                           QVector3D translation,
                           QVector3D scaling) {
    // texture index 1 is the tile
    VisualObject * object = new VisualObject(this->wall,
                                             this->textures.value(4),
                                             translation, rotation,
                                             "Wall");
    object->setScaling(scaling);
    // set horizontal centers
    object->setLMidHorisontal(this->center_h_1);
    object->setUMidHorisontal(this->center_h_2);
    this->models.push_back(object);
    object->setTextureID(704);
    object->setTexturePath("://Texture4");
    PremisesExporter::export_environment(this->models, "environment.env");
    PremisesExporter::export_texture(this->texture_paths, "textures.tl");

}

void RenderState::add_door(QVector3D rotation,
                           QVector3D translation,
                           QVector3D scaling) {
    // texture index 1 is the tile
    VisualObject * object = new VisualObject(this->door,
                                             this->textures.value(2),
                                             translation,
                                             rotation,
                                             "Door");
    object->setScaling(scaling);
    object->setTextureID(702);
    object->setTexturePath("://Texture2");
    this->models.push_back(object);
    PremisesExporter::export_environment(this->models, "environment.env");
    PremisesExporter::export_texture(this->texture_paths, "textures.tl");

}

void RenderState::add_floor_plan(QVector3D rotation,
                                 QVector3D translation,
                                 QVector3D scaling) {
    // texture index 1 is the tile
    VisualObject * object =
            new VisualObject(this->plane,
                             this->
                             textures_from_files.
                             value(this->
                                   textures_from_files.count() -
                                   1),
                             translation,
                             rotation,
                             "FloorPlan");
    object->setScaling(scaling);
    object->setTextureID(this->textures_from_files.count() - 1);
    object->setTexturePath(this->
                           texture_paths.value(this->
                                               textures_from_files.count() -
                                               1));
    this->models.push_back(object);
    PremisesExporter::export_environment(this->models, "environment.env");
    PremisesExporter::export_texture(this->texture_paths, "textures.tl");


}

void RenderState::resizeGL(int w, int h) {
    // setup the viewport for opengl
    glViewport(0, 0, w, h);

    // initialize the projection matrix
    pMatrix.setToIdentity();

    // set the projection matrix
    pMatrix.perspective(10,
                        static_cast<float>(w) /
                        static_cast<float>(h),
                        1.0f,
                        1000.0f);
}

void RenderState::LoadContent() {
    // this initializes all the opengl functions
    initializeOpenGLFunctions();

    // load meshes
    this->node = new ModelMesh("://Sphere");
    this->plane = new ModelMesh("://Plane");
    this->door = new ModelMesh("://DoorWay01");
    this->wall = new ModelMesh("://Wall01");
    this->tree = new ModelMesh("://Tree01");

    // load shaders
    this->program = new QOpenGLShaderProgram();

    // load vertex shader (the geometry of the 3D objects )
    this->program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           "://Vertex");

    // load the pixel/fragment shader.
    // this is the pixel shader (per pixel rendering)
    this->program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           "://Fragment");

    // link the shaders
    this->program->link();
}

void RenderState::receive_config() {

}

void RenderState::receive_directories() {

}

void RenderState::paintGL() {
  // initialise the view matrix
  this->vMatrix.setToIdentity();
  // whenever content is not loaded, load the content
  if ( !this->program ) {
      LoadContent();
      // checkout opengl context
      emit opengl_initialised(this->context()->isValid());
  }
  // enable the scene's depth mask
  glDepthMask(GL_TRUE);
  // clear the depth z = 0.0f -> 1.0f
  glClearDepth(1.0f);
  // enable the scene's depth test
  glEnable(GL_DEPTH_TEST);
  // enable cullmode CCW (counter clockwise)
  glEnable(GL_CULL_FACE);
  // enable transparency
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // clear the background color for rendering
  // cornflower blue 659CEF
  glClearColor(210.0/255.0, 210.0/255.0, 210.0/255.0, 1);
  // clear the color and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  // setup camera
  QMatrix4x4 cameraTransformation;
  // rotation in the y - axis
  cameraTransformation.rotate(0, 0, 1, 0);
  // rotation in the x - axis
  cameraTransformation.rotate(-90, 1, 0, 0);
  // transform the camera's position with respect to the rotation matrix
  QVector3D cameraPosition = cameraTransformation *
          QVector3D(0,
                    0,
                    this->mouse_zoom +
                    this->current_floor_height);
  // define the direction of the camera's up vector
  QVector3D cameraUpDirection = cameraTransformation *
          QVector3D(0, 1, 0);
  // implement and transform the camera
  this->vMatrix.lookAt(cameraPosition,
                 QVector3D(0,
                           this->current_floor_height,
                           0),
                 cameraUpDirection);
  this->vMatrix.translate(this->camera_prev);
  // return the position of the ray intersection with the y-axis
  QVector3D Pos  = Mathematics::intersectYat(this->raycast,
                                             QVector3D(0,
                                                       this->mouse_zoom,
                                                       0) -
                                             this->camera_prev,
                                             this->current_floor_height);
  // update current position
  this->current_position->setX(Pos.x());
  this->current_position->setZ(Pos.z());
  this->current_position->setY(Pos.y());

  // draw other objects first
  foreach(VisualObject *object, this->models) {
    QVector3D color = QVector3D();
    QMatrix4x4 translation;
    translation.translate(object->getTranslation());
    QMatrix4x4 rotation;
    rotation.rotate(object->getRotation().y(), 0, 1, 0);
    rotation.scale(object->getScaling());
    if ( object->getType().compare("FloorPlan") != 0 ) {
      DrawGL::DrawModel(object->getModelMesh(),
                        this->vMatrix, translation,
                        rotation, object->getTexture(),
                        QVector3D(),
                        QVector2D(object->getScaling().z(),
                                  object->getScaling().x()),
                        this->program,
                        pMatrix,
                        this->current_floor_height);
    } else {

      if ( Mathematics::detect_point_in_plan_on_y(
               object->getTranslation(),
               object->getScaling(),
                     object->getRotation().y(),
                     QVector3D(this->current_position->x(),
                               this->current_position->y(),
                               this->current_position->z())) ) {
          if ( this->floor_plan_removable )
            color = QVector3D(1, 0, 0);
          if ( this->edit_floorplan )
            color = QVector3D(1, 0.5, 0.25);

      } else {
          if ( this->selected_floor_plan > -1 &&
               this->selected_floor_plan < this->models.count() &&
               this->edit_floorplan &&
              (this->models.value(this->selected_floor_plan) ==
               object) )
              color = QVector3D(1, 0.5, 0.25);
          else
          color = QVector3D();
      }
      DrawGL::DrawModel(object->getModelMesh(),
                        this->vMatrix, translation,
                        rotation, object->getTexture(),
                        color, QVector2D(1, 1),
                        this->program, pMatrix,
                        this->current_floor_height);
    }
      if ( this->wall_placable ) {
        if ( object->getLMidHorisontal().distanceToPoint(Pos) < 0.25f )
          *this->current_position = object->getLMidHorisontal();
        if ( object->getUMidHorisontal().distanceToPoint(Pos) < 0.25f )
          *this->current_position = object->getUMidHorisontal();
      }
  }

  // draw placable objects here
  DrawPlacableItems(Pos);
  // draw all the nodes here
  DrawNodes();
  // draw all the lines connected to nodes with directional arrows
  DrawNodeLines(Pos);
  // draw lines last
  DrawObjectLines(Pos);
  // draw left clicked line(s)
  if ( (this->node_linkable) &&
       (this->mousedown_left) &&
       (this->node_index_selected > -1) &&
       (this->node_index_selected < this->nodes.count()) ) {
    QVector3D aux_calc_one, aux_calc_two, aux_angle;
    QMatrix4x4 aux_rotate, aux_45;
    aux_angle = this->nodes.value(this->node_index_selected)->Position() -
            *this->current_position;
    aux_angle.setY(0);

    // get the angle from the arccos function
    if ( aux_angle.z() > 0 ) {
      aux_rotate.rotate(45 - 180 *  acos(aux_angle.x() / aux_angle.length()) /
                        (3.141592), 0, 1, 0);
    } else {
      aux_rotate.rotate(45 + 180 * acos(aux_angle.x() / aux_angle.length()) /
                        (3.141592), 0, 1, 0);
    }
    aux_45.rotate(90, 0, 1, 0);
    aux_calc_one = aux_rotate*(QVector3D(0, 0, 1));
    aux_calc_two = aux_45*aux_rotate*(QVector3D(0, 0, 1));

    DrawGL::DrawLine(this->nodes.value(this->node_index_selected)->Position(),
                     *this->current_position, this->vMatrix,
                     QMatrix4x4(), QMatrix4x4(), QVector3D(1, 1, 0),
                     this->program, pMatrix, this->current_floor_height);
    DrawGL::DrawLine((this-> nodes.value(this->node_index_selected)->
                      Position() +  *this->current_position) / 2.0,
                     aux_calc_one + (this->nodes.value(
                                         this->node_index_selected)->
                                     Position() +
                                     *this->current_position) / 2.0,
                     this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                     QVector3D(1, 1, 0), this->program, pMatrix,
                     this->current_floor_height);
    DrawGL::DrawLine((this->nodes.value(this->node_index_selected)->
                      Position() + *this->current_position) / 2.0,
                     aux_calc_two +
                     (this->nodes.value(this->node_index_selected)->
                      Position() + *this->current_position) / 2.0,
                     this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                     QVector3D(1, 1, 0), this->program, pMatrix,
                     this->current_floor_height);
  }
  // release the program for this frame
  this->program->release();
  // disable the cullmode for the frame
  glDisable(GL_CULL_FACE);
  // disable the depthtest for the frame
  glDisable(GL_DEPTH_TEST);
  // finish up the opengl frame
  glFinish();

  // draw the node text to the scene
  DrawNodeNames();
}

void RenderState::DrawObjectLines(QVector3D Pos) {
  // draw circles around selected objects
  foreach(VisualObject *object, this->models) {
      if ( (this->tree_removable)
           && (object->getType().compare("Tree", Qt::CaseInsensitive) == 0)
           && (object->getTranslation().distanceToPoint(Pos) < 4.0) ) {
          // draw a circle here
          draw_circle_flat(object->getTranslation(),
                           this->vMatrix,
                           QVector3D(1, 0, 0),
                           4.0f);
      }
      if ( this->wall_placable ) {
          if ( object->getLMidHorisontal().distanceToPoint(Pos) < 0.25f )
              draw_circle_flat(object->getLMidHorisontal(),
                               this->vMatrix,
                               QVector3D(0, 1, 0),
                               0.25f);
          if ( object->getUMidHorisontal().distanceToPoint(Pos) < 0.25f )
              draw_circle_flat(object->getUMidHorisontal(),
                               this->vMatrix,
                               QVector3D(0, 1, 0),
                               0.25f);
     }
  }
}

void RenderState::DrawPlacableItems(QVector3D Pos) {
    // draw placable node
    DrawGL::draw_if_true(this->node, this->vMatrix,
                         Pos, this->rotation,
                         QVector3D(0.51,0.51,0.51),
                         this->textures.value(0),
                         QVector3D(1, 0, 0),
                         QVector2D(1, 1),
                         pMatrix, this->program,
                         this->node_placable
                         && this->node_significant,
                         this->current_floor_height);

    // draw placable node (not significant)
    DrawGL::draw_if_true(this->node, this->vMatrix,
                         Pos, this->rotation,
                         QVector3D(0.5, 0.5, 0.5),
                         this->textures.value(0),
                         QVector3D(), QVector2D(1, 1),
                         pMatrix, this->program,
                         this->node_placable
                         && !this->node_significant,
                         this->current_floor_height);

    // draw placable tile draggable mouse
    DrawGL::draw_if_true(this->plane, this->vMatrix,
                         Pos, this->rotation,
                         QVector3D(1, 1, 1),
                         this->textures.value(1),
                         QVector3D(),
                         QVector2D(this->currentscale.z(),
                                   this->currentscale.x()),
                         pMatrix, this->program,
                         this->pavement_placable
                         && (!this->mousedown_left),
                         this->current_floor_height);
    // draw placable tile clicked
    if ( this->mousedown_left && this->pavement_placable ) {
      this->currentscale.setZ(pow(pow((this->clicked_position->z() -
                                       this->current_position->z()), 2),
                                  0.5) * 2.0);
      this->currentscale.setY(1);
      this->currentscale.setX(pow(pow((this->clicked_position->x() -
                                       this->current_position->x()), 2),
                                  0.5) * 2.0);
    }
    // draw pavement
    DrawGL::draw_if_true(this->plane, this->vMatrix,
                         *this->clicked_position,
                         this->rotation, this->currentscale,
                         this->textures.value(1), QVector3D(),
                         QVector2D(this->currentscale.z(),
                                   this->currentscale.x()),
                         pMatrix, this->program,
                         this->pavement_placable
                         && (this->mousedown_left),
                         this->current_floor_height);

    // draw placable door
    DrawGL::draw_if_true(this->door, this->vMatrix,
                         Pos, this->rotation,
                         QVector3D(1, 1, 1),
                         this->textures.value(2),
                         QVector3D(), QVector2D(1, 1),
                         pMatrix, this->program,
                         this->door_placeable,
                         this->current_floor_height);

    // draw placable wall
    if ( (this->mousedown_left) && (this->wall_placable) ) {
      this->drag_middle_position = (*this->clicked_position +
                                    *this->current_position) / 2.0;
      this->rotation.setY(Mathematics::flat_angle_from_vectors(
                              *this->clicked_position,
                              *this->current_position) + 90);
  // clamp to 0 and 180 degrees
  if ( (Mathematics::return_near_degree(this->rotation.y()) == 0.0)
       || (Mathematics::return_near_degree(this->rotation.y()) == 180) ) {
      // set fixed rotation for the rotation
      this->rotation.setY(Mathematics::return_near_degree(this->rotation.y()));

      // set fixed position for the  x - axis
      this->drag_middle_position.setX(this->clicked_position->x());
      this->current_position->setX(this->clicked_position->x());

      DrawGL::DrawLine(*this->clicked_position +
                       QVector3D(0, 0, -infinte_lenght_lines),
                       *this->current_position +
                       QVector3D(0, 0, infinte_lenght_lines),
                       this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                       QVector3D(1, 1, 1), this->program, pMatrix,
                       this->current_floor_height);
    }

        // clamp to 270 and 90 degrees
        if ( (Mathematics::return_near_degree(this->rotation.y()) == 270)
             || (Mathematics::return_near_degree(this->rotation.y()) == 90)
             || (Mathematics::return_near_degree(this->rotation.y()) == -90) ) {
          this->rotation.setY(Mathematics::return_near_degree(this->
                                                                rotation.y()));
          this->drag_middle_position.setZ(this->clicked_position->z());
          this->current_position->setZ(this->clicked_position->z());
          DrawGL::DrawLine(*this->clicked_position +
                           QVector3D(-infinte_lenght_lines, 0, 0),
                           *this->current_position +
                           QVector3D(infinte_lenght_lines, 0, 0),
                           this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                           QVector3D(1, 1, 1), this->program, pMatrix,
                           this->current_floor_height);
        }
        // set clickable centers
        this->center_h_1 = *this->current_position;
        this->center_h_2 = *this->clicked_position;

        this->currentscale.setZ(this->clicked_position->
                                  distanceToPoint(*this->current_position));
        DrawGL::draw_if_true(this->wall,  this->vMatrix,
                             this->drag_middle_position, this->rotation,
                             QVector3D(1, 1, this->currentscale.z()),
                             this->textures.value(4), QVector3D(),
                             QVector2D(this->currentscale.z(), 1.0),
                             pMatrix, this->program, this->wall_placable,
                             this->current_floor_height);
      }

    // draw placable tree
    DrawGL::draw_if_true(this->tree, this->vMatrix, Pos,
                         this->rotation, QVector3D(1, 1, 1),
                         this->textures.value(3),
                         QVector3D(), QVector2D(1, 1),
                         pMatrix, this->program,
                         this->tree_placable,
                         this->current_floor_height);

    // draw placable floorplan
    DrawGL::draw_if_true(this->plane, this->vMatrix, Pos,
                         this->rotation, this->currentscale,
                         this->textures_from_files.
                         value(this->textures_from_files.count() - 1),
                         QVector3D(), QVector2D(1, 1), pMatrix,
                         this->program, this->placable_floor_plan
                         && (this->textures_from_files.count() > 0),
                         this->current_floor_height);
}

void RenderState::DrawNodeNames() {
  // draw the nodes to the scene
  QPainter painter;
  painter.begin(this);
  painter.setPen(Qt::white);
  painter.setFont(QFont("Arial", 8));
  //painter.setRenderHints(QPainter::Antialiasing |
  //                        QPainter::SmoothPixmapTransform);
  // draw all the node text here
  foreach(Node *n, this->nodes) {
    if ( ( n->Position().y() < this->current_floor_height + 0.5 ) &&
         ( n->Position().y() > this->current_floor_height - 0.5 ) ) {
      QPoint pos_x_y =
            Mathematics::transform_3d_to_2d(this->vMatrix, this->pMatrix,
                                            n->Position(), this->width(),
                                            this->height());
      painter.drawText(pos_x_y.x(), pos_x_y.y(), n->getName());
    }
  }
  painter.end();
}

void RenderState::DrawNodes() {

  // draw all the nodes here
  foreach(Node *n, this->nodes) {
    QMatrix4x4 translation;


        translation.translate(n->Position());
        translation.scale(0.5);
        DrawGL::DrawModel(this->node, this->vMatrix,
                        translation, QMatrix4x4(),
                        this->textures.value(0),
                        QVector3D(), QVector2D(1, 1),
                        this->program, this->pMatrix,
                          this->current_floor_height);

  }
}

void RenderState::clear_premises() {
  // clear the nodes
  this->nodes.clear();

  // clear visual objects
  this->models.clear();

  // clear the textures
  for ( int l = 1; l < this->textures.count(); l++ )
   this->textures.removeAt(l);
}

void RenderState::DrawNodeLines(QVector3D Pos) {
  // draw all the node lines here
  foreach(Node *n, this->nodes) {
    if ( n->Position().distanceToPoint(Pos) < 0.5 ) {
      // draw red circle to indicate the node will be removed
      if ( this->node_removable)
        draw_circle_flat(n->Position(), this->vMatrix,
                         QVector3D(1, 0, 0), 0.7f);

      // draw green circle to indicate a link will be added
      if ( this->node_linkable)
        draw_circle_flat(n->Position(), this->vMatrix,
                         QVector3D(0, 1, 0), 0.7f);

      // draw green circle to indicate a link will be added
      if ( this->edit_node )
        draw_circle_flat(n->Position(), this->vMatrix,
                         QVector3D(1, 0.5, 0.25), 0.7f);
    }
    if ( n->Position().distanceToPoint(*this->clicked_position) < 0.5 ) {
      // draw green circle to indicate a link will be added
      if ( this->node_linkable)
        draw_circle_flat(n->Position(), this->vMatrix,
                         QVector3D(0, 1, 0), 0.7f);
    }
    for ( int i = 0; i  < n->countConnected(); i++ ) {
        QVector3D color = QVector3D(0, 1, 0);
        if ( Mathematics::detect_point_near_line(n->Position(),
             this->nodes.value(n->getConnectedIndex(i))->Position(),
             QVector3D(this->current_position->x(),
                       this->current_position->y(),
                       this->current_position->z()),
                                                 0.5) &&
             this->link_removable) {
            color = QVector3D(1, 0, 0);
        } else {
            color = QVector3D(0, 1, 0);
        }
      if ( n->getConnectedIndex(i) < this->nodes.count() ) {
        QVector3D aux_calc_one, aux_calc_two, aux_angle;
        QMatrix4x4 aux_rotate, aux_45;
        aux_angle = n->Position() -
                this->nodes.value(n->getConnectedIndex(i))->Position();
        aux_angle.setY(0);

        // get the angle from the arccos function
        if ( aux_angle.z() > 0 )
          aux_rotate.rotate(45 - 180 *
                            acos(aux_angle.x() / aux_angle.length()) /
                            (3.141592),
                            0,
                            1,
                            0);
        else
            aux_rotate.rotate(45 +
                              180 *
                              acos(aux_angle.x() / aux_angle.length()) /
                              (3.141592),
                              0,
                              1,
                              0);

            aux_45.rotate(90, 0, 1, 0);
            aux_calc_one = aux_rotate * (QVector3D(0, 0, 0.25));
            aux_calc_two = aux_45 * aux_rotate * (QVector3D(0, 0, 0.25));

            DrawGL::DrawLine(n->Position(),
                             this->nodes.value(
                                   n->getConnectedIndex(i))->Position(),
                             this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                             color, this->program, pMatrix,
                             this->current_floor_height);
            DrawGL::DrawLine((n->Position() +
                              this->nodes.value(n->getConnectedIndex(i))->
                              Position()) / 2.0,
                             aux_calc_one +
                             (n->Position() +
                              this->nodes.value(n->getConnectedIndex(i))->
                              Position()) / 2.0,
                             this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                             color, this->program, pMatrix,
                             this->current_floor_height);
            DrawGL::DrawLine((n->Position() +
                              this->nodes.value(n->getConnectedIndex(i))->
                              Position()) / 2.0,
                             aux_calc_two +
                             (n->Position() +
                              this->nodes.value(n->getConnectedIndex(i))->
                              Position()) / 2.0,
                             this->vMatrix, QMatrix4x4(), QMatrix4x4(),
                             color, this->program, pMatrix,
                             this->current_floor_height);
      }
    }
  }
}

void RenderState::draw_circle_flat(QVector3D center,
                                   QMatrix4x4 wvp,
                                   QVector3D color,
                                   float radius) {
    const int slices = 6;
    for ( int k = 0; k < slices; k++ ) {
        DrawGL::DrawLine(
                    radius * QVector3D(cos(2 * 3.14 * k / slices),
                    0,
                    sin(2 * 3.14 * k / slices)) + center,
                    radius * QVector3D(cos(2 * 3.14 * (k + 1) / slices),
                    0,
                    sin(2 * 3.14 * (k + 1) / slices)) + center,
                    wvp,
                    QMatrix4x4(),
                    QMatrix4x4(),
                    color,
                    this->program,
                    pMatrix, this->current_floor_height);
    }
}

void RenderState::LoadTextures(QString path) {
  // clear the premises when not empty
  if ( this->textures_from_files.count() > 0 ) {
    this->textures_from_files.clear();
    this->texture_paths.clear();
  }
  /* populate the textures from the text file */
  if ( PremisesExporter::fileExists(path + "textures.tl") ) {

    // load the text file
    QFile textfile(path + QString("textures.tl"));
    // open the text file
    textfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream ascread(&textfile);

    if ( textfile.isOpen() ) {
      // read each line of the file
      QString line = ascread.readLine();
      while ( !line.isNull() ) {
        // break the line up in usable parts
        QStringList list = line.split(",");

        // check the type of line
        if ( list[0] == "t" ) {
          int texture_index = 0;
          QString texture_path = "";

          // texture type
          texture_path = list[2];
          QStringList ls = texture_path.split("/");

          // texture index
          QTextStream(&list[1]) >> texture_index;
          QString new_path = path + ls[ls.count() - 1];
          load_texture_from_file(new_path);
        }

      // read next line
      line = ascread.readLine();
      }

      // close the textfile
      textfile.close();
    }
  }
}

void RenderState::LoadObjects(QString path) {
    // clear the premises when not empty
    if ( this->models.count() > 0)
        this->models.clear();

    /* populate the premisis from the text file */

    // load the text file
    QFile textfile(path + QString("environment.env"));

    // open the text file
    textfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream ascread(&textfile);

    if ( textfile.isOpen() ) {
        // read each line of the file
        QString line = ascread.readLine();

        while ( !line.isNull() ) {
            // break the line up in usable parts
            QStringList list = line.split(",");

            // check the type of line
            /* n-> node
             * j-> join
             */
            if ( list[0] == "o" ) {
                // this is only x, y, z coordinates for the node
                float matrix[9];
                int texture_index = 0;
                QString Type ="";

                // populate the vertices
                for ( int i = 0; i < 9; i++ )
                     QTextStream(&list[i+3]) >> matrix[i];

                // texture index
                QTextStream(&list[12]) >> texture_index;

                // object type
                QTextStream(&list[2]) >> Type;
                // add floorplan
                if ( Type.compare("FloorPlan", Qt::CaseInsensitive) == 0 ) {
                    if ( texture_index < this->textures_from_files.count() ) {
                      VisualObject *object =
                        new VisualObject(this->plane,
                                         this->
                                         textures_from_files[texture_index],
                                         QVector3D(matrix[0],
                                                   matrix[1],
                                                   matrix[2]),
                                         QVector3D(matrix[3],
                                                   matrix[4],
                                                   matrix[5]),
                                         "FloorPlan");
                    object->setScaling(QVector3D(matrix[6],
                                                 matrix[7],
                                                 matrix[8]));
                    object->setTextureID(texture_index);
                    this->models.push_back(object);
                    } else {
                      QMessageBox::warning(
                                  this,
                                  tr("Texture file error"),
                                  tr("The texture file may be corrupted\n"
                                  "or the textures are not located in the "
                                  "same folder as the environment file."));
                    }
                }
            }

            // read next line
           line = ascread.readLine();
        }

        // close the textfile
        textfile.close();
    }
}

void RenderState::LoadNodes(QString filename) {
    QVector<int> walk, wheelchair, vehicle, bicycle;
    walk.clear();
    wheelchair.clear();
    vehicle.clear();
    bicycle.clear();
    // clear the premises when not empty
    if ( this->nodes.count() > 0 )
        this->nodes.clear();

    /* populate the premisis from the text file */

    // load the text file
    QFile textfile(filename + QString("nodes.pvc"));

    // open the text file
    textfile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream ascread(&textfile);

    if ( textfile.isOpen() ) {
        // read each line of the file
        QString line = ascread.readLine();

        while ( !line.isNull() ) {
            // break the line up in usable parts
            QStringList list = line.split(",");

            // check the type of line
            /* n-> node
             * j-> join
             */
            if ( list[0] == "n" ) {
                // this is only x, y, z coordinates for the node
                float vertex[3];
                int signi = 0;
                QString display_name ="";
                // populate the vertices
                for ( int i = 0; i < 3; i++ )
                     QTextStream(&list[i+2]) >> vertex[i];

                // get node significance
                QTextStream(&list[6]) >> signi;
                // get the node's name
                display_name = list[5];
                Node* n = new Node(new QVector3D(vertex[0],
                                                 vertex[1],
                                                 vertex[2]));
                // initialize node paths
                n->setWheelChair(false);
                n->setBike(false);
                n->setWalk(false);
                n->setVehicle(false);
                // set node's significance
                n->setSignificant((signi == 1));
                // set node's name
                n->setName(display_name);
                // add the node to the premises
                this->nodes.push_back(n);
            } else if ( list[0] == "j" ) {
                   // this is only the indices that should be join
                    int uv[2];

                    // populate the indices
                    for ( int i = 0; i < 2; i++ )
                         QTextStream(&list[i+1]) >> uv[i];

                    QString p = this->nodes.value(uv[1])->getName();
                    // add the links
                    this->nodes.value(uv[0])->AddLink(&p, uv[1]);

             } else if ( list[0] == "wc" ) {
                if(list.count() > 1)
                wheelchair.append(list[1].toInt());
            } else if ( list[0] == "vi" ) {
                if(list.count() > 1)
                vehicle.append(list[1].toInt());
            } else if ( list[0] == "ft" ) {
                if(list.count() > 1)
                walk.append(list[1].toInt());
            } else if ( list[0] == "by" ) {
                if(list.count() > 1)
                bicycle.append(list[1].toInt());
            }
            // read next line
           line = ascread.readLine();
        }

        // close the textfile
        textfile.close();
    }
    // add walkable nodes
    for ( int i = 0; i < walk.count(); i++) {
        this->nodes.value(walk.value(i))->setWalk(true);
    }
    // add wheelchair nodes
    for ( int i = 0; i < wheelchair.count(); i++) {
        this->nodes.value(wheelchair.value(i))->setWheelChair(true);
    }
    // add vehicle nodes
    for ( int i = 0; i < vehicle.count(); i++) {
        this->nodes.value(vehicle.value(i))->setVehicle(true);
    }
    // add bicycle nodes
    for ( int i = 0; i < bicycle.count(); i++) {
        this->nodes.value(bicycle.value(i))->setBike(true);
    }
  // show node errors
  update_node_errors();
}

void RenderState::CopyConfig(QString value) {
    QString val_new = "VirtualConcierge/config.config";

    QDir dir;
    // try to copy the config to the drive
    if ( QString::compare(dir.absolutePath() + "/" + val_new,
                          value,
                          Qt::CaseInsensitive) != 0 ) {
      if ( QFile::exists(val_new) && !start_up_load_tex ) {
       if ( !QFile::remove(val_new) ) {

       }
      }
      if ( QFile::exists(value) )
      if ( !QFile::copy(value, val_new) ) {
        if ( !QFile::exists(val_new) ) {
          QMessageBox::warning(this,
                               tr("Error file copying"),
                               tr("Texture file could not"
                                  " be copied to the drive."));
        }
      }
    }
}

void RenderState::CopyDirectories(QString value) {
    QString val_new = "VirtualConcierge/directories.dir";

    QDir dir;
    // try to copy the texture to the drive
    if ( QString::compare(dir.absolutePath() + "/" + val_new,
                          value,
                          Qt::CaseInsensitive) != 0 ) {
      if ( QFile::exists(val_new) && !start_up_load_tex ) {
       if ( !QFile::remove(val_new) ) {
          // QMessageBox::warning(this,
          //                      tr("Error file deleting"),
          //                      tr("Texture file could not"
          //                        " be deleted from the drive."));
       }
      }
      if ( QFile::exists(value) )
      if ( !QFile::copy(value, val_new) ) {
        if ( !QFile::exists(val_new) ) {
          QMessageBox::warning(this,
                               tr("Error file copying"),
                               tr("Texture file could not"
                                  " be copied to the drive."));
        }
      }
    }
}

RenderState::~RenderState() {
  delete this->program;
  delete this->position;
  delete this->clicked_position;
  delete this->node;
  delete this->plane;
  delete this->wall;
  delete this->door;
  delete this->tree;
  delete this->current_position;
}
