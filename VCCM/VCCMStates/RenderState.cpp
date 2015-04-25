/** *********************************************************************** **/
/** *********************************************************************** **/
/**     Created by:     Ruan (Baggins) Luies 23511354                       **/
/**     email:          23511354@nwu.ac.za                                  **/
/**     Project name:   Virtual Concierge Creator And Management System     **/
/**     File Name:      RenderState.cpp                                     **/
/**     From Date:      2015/02/24                                          **/
/**     To Date:        2015/10/01                                          **/
/** **********************************************************************  **/
/** *********************************************************************** **/

#include "VCCMStates/RenderState.h"
#include <qdebug.h>

RenderState::RenderState(): m_program(0), m_t(0) {
    m_position = new QVector3D(0,0,0);
    m_handler = new NodeHandler();
    for(int k = -5; k < 5;k++)
        for(int j = -5; j < 5;j++)
            m_handler->AddNode(new Node(new QVector3D(4*k,0,4*j)));

    for(int k = 0; k < 99;k++)
    {
        if(k%10 !=9)
        {
            m_handler->AddNodeLinkbyIndex(k,k+1);
            m_handler->AddNodeLinkbyIndex(k+1,k);
        }

        if(k<90)
            m_handler->AddNodeLinkbyIndex(k,k+10);

        if(k>10)
            m_handler->AddNodeLinkbyIndex(k,k-10);


    }

    //m_handler->ReadFilePVC(":/Premises");
    m_handler->CalculateShortest(54,93);
}

void RenderState::paint()
{
    QMatrix4x4 vMatrix; // define a view matrix
    // whenever content is not loaded, load the content
    if(!m_program){LoadContent();}
    // set the uniform value t for the shader (give it a new value)
    m_program->setUniformValue("t", (float) m_t);
    // setup the viewport for opengl
    glViewport(0, 0, m_viewportSize.width(), m_viewportSize.height());
    // initialize the projection matrix
    pMatrix.setToIdentity();
    // set the projection matrix
    pMatrix.perspective(60, (float) m_viewportSize.width() / (float) m_viewportSize.height(), 1.0f, 1000.0f);
    // enable the scene's depth mask
    glDepthMask(GL_TRUE);
    // clear the depth z = 0.0f -> 1.0f
    glClearDepth(1.0f);
    // enable the scene's depth test
    glEnable(GL_DEPTH_TEST);
    // enable cullmode CCW (counter clockwise)
    glEnable(GL_CULL_FACE);
    // clear the background color for rendering
    glClearColor(0.97, 0.97, 0.97, 1);
    // clear the color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // setup camera
    QMatrix4x4 cameraTransformation;
    // rotation in the y - axis
    cameraTransformation.rotate(0, 0, 1, 0);
    // rotation in the x - axis
    cameraTransformation.rotate(-90, 1, 0, 0);
    // transform the camera's position with respect to the rotation matrix
    QVector3D cameraPosition = cameraTransformation * QVector3D(m_position->x(), 0, 60.0f);
    // define the direction of the camera's up vector
    QVector3D cameraUpDirection = cameraTransformation * QVector3D(0, 1, 0);
    // implement and transform the camera
    vMatrix.lookAt(cameraPosition, QVector3D(0, 0, 0), cameraUpDirection);
    /// This is test code for drawing a line
    //DrawLine(QVector3D(0,0,0),QVector3D(5,0,5),vMatrix,QMatrix4x4(),QMatrix4x4(),QVector3D(0,0,0));
    /// This is test code for drawing another line
    //DrawLine(QVector3D(0,0,0),QVector3D(-5,0,-5),vMatrix,QMatrix4x4(),QMatrix4x4(),QVector3D(0,0,0));

    // draw each node to the scene
    for(int x = 0; x<m_handler->count();x++)
    {
      for(int l = 0;l<m_handler->NodeFromIndex(x).countConnected();l++){
      DrawLine(m_handler->NodeFromIndex(x).Position(),m_handler->NodeFromIndex(m_handler->NodeFromIndex(x).getConnectedIndex(l)).Position(),
               vMatrix,QMatrix4x4(),QMatrix4x4(),QVector3D(0,0,0));
      //qDebug()<<"connected:"<<m_handler->NodeFromIndex(m_handler->NodeFromIndex(x).getConnectedIndex(l)).Position();
      //qDebug()<<m_handler->NodeFromIndex(x).Position();
      }
      // this is for the model transformation
      QMatrix4x4 mMatrix;
      // transform the position locally
      mMatrix.translate(m_handler->NodeFromIndex(x).Position());
      // draw different types of nodes, (connected & unconnected nodes & best path)
      DrawModel(node,vMatrix,mMatrix,QMatrix4x4()/*,0*/,m_handler->NodeFromIndex(x).getColor());
    }

    for(int o = 0;o<m_handler->pathcount()-1;o++){
    DrawLine(m_handler->NodeFromIndex(m_handler->pathindex(o)).Position(),m_handler->NodeFromIndex(m_handler->pathindex(o+1)).Position(),
             vMatrix,QMatrix4x4(),QMatrix4x4(),QVector3D(0,1,0));
    //qDebug()<<"connected:"<<m_handler->NodeFromIndex(m_handler->NodeFromIndex(x).getConnectedIndex(l)).Position();
    //qDebug()<<m_handler->NodeFromIndex(x).Position();
    }
    // release the program for this frame
    m_program->release();
    // disable the cullmode for the frame
    glDisable(GL_CULL_FACE);
    // disable the depthtest for the frame
    glDisable(GL_DEPTH_TEST);
    // finish up the opengl frame
    glFinish();
}

void RenderState::LoadContent()
{
    // this initializes all the opengl functions
    initializeOpenGLFunctions();
    //load meshes
    box = new ModelMesh(":/block");
    node = new ModelMesh(":/Node");
    sky= new ModelMesh(":/Sky");
    wagen = new ModelMesh(":/Cube");
    // load shaders
    m_program = new QOpenGLShaderProgram();
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,":/VertexShader");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,":/FragmentShader");
    m_program->link();
}

void RenderState::UpdateShaders(QMatrix4x4 wvp,QMatrix4x4 mvp, QMatrix4x4 rotate/*, GLuint texture*/,QVector3D color)
{
    //glBindTexture(GL_TEXTURE_2D, texture);
    m_program->bind();// bind the current shader code
    m_program->setUniformValue("col",color);
    m_program->setUniformValue("rotationMatrix", rotate);
    m_program->setUniformValue("mvpMatrix", mvp * rotate);// model view projection
    m_program->setUniformValue("wvpMatrix", pMatrix * wvp);// model view projection
    //shaderProgram->setUniformValue("texture", 0);// use GL_TEXTURE0
}

void RenderState::ShaderDraw(ModelMesh *box)
{

    const char *vert ="vertex";//= vertex.toStdString().c_str();// convert the qstring to c-string for opengl purposes
    //const char *textureCoordinate= "textureCoordinate";//= texCoord.toStdString().c_str();// convert the qstring to c-string for opengl purposes
    const char *normals = "normal";// convert the qstring to c-string for opengl purposes
    m_program->setAttributeArray(vert, box->vertices.constData());//load the vertices to the shaders
    m_program->enableAttributeArray(vert);//enable the shader attribute( vertices )
    m_program->setAttributeArray(normals, box->normals.constData());//load the normals to the shaders
    m_program->enableAttributeArray(normals);//enable the shader attribute( normals )
       //m_program->setAttributeArray(textureCoordinate, box->textureCoordinates.constData());//load the texture coordinates to the shaders
      // m_program->enableAttributeArray(textureCoordinate);//enable the shader attribute( texture coordinates )
       box->Draw(); // draw the opengl vertices
       m_program->disableAttributeArray(vert);// disable the vertex attributes
       m_program->disableAttributeArray(normals);// disable the normal attributes
       //m_program->disableAttributeArray(textureCoordinate);// disable the Texture coordinates attributes

       m_program->release(); // release the current updated shader code (awaiting next frame)
   }

void RenderState::DrawLine(QVector3D point1, QVector3D point2,QMatrix4x4 wvp,QMatrix4x4 mvp, QMatrix4x4 rotate/*, GLuint texture*/,QVector3D color)
   {
       QVector< QVector3D > temp_vertices;
       temp_vertices.push_back(point1);
       temp_vertices.push_back(point2);
   UpdateShaders(wvp, mvp, rotate/*, texture*/, color);
    const char *vert ="vertex";//= vertex.toStdString().c_str();// convert the qstring to c-string for opengl purposes
    //const char *textureCoordinate= "textureCoordinate";//= texCoord.toStdString().c_str();// convert the qstring to c-string for opengl purposes
    const char *normals = "normal";// convert the qstring to c-string for opengl purposes
    m_program->setAttributeArray(vert, temp_vertices.constData());//load the vertices to the shaders
    m_program->enableAttributeArray(vert);//enable the shader attribute( vertices )
    m_program->setAttributeArray(normals, temp_vertices.constData());//load the normals to the shaders
    m_program->enableAttributeArray(normals);//enable the shader attribute( vertices )
    //m_program->setAttributeArray(textureCoordinate, box->textureCoordinates.constData());//load the texture coordinates to the shaders
   // m_program->enableAttributeArray(textureCoordinate);//enable the shader attribute( texture coordinates )
    glLineWidth(2.0);
    glDrawArrays(GL_LINES, 0, temp_vertices.size());

    m_program->disableAttributeArray(vert);// disable the vertex attributes
    m_program->disableAttributeArray(normals);// disable the normal attributes
    m_program->release(); // release the current updated shader code (awaiting next frame)
    temp_vertices.clear();
}

void RenderState::DrawModel(ModelMesh *box,QMatrix4x4 wvp,QMatrix4x4 mvp, QMatrix4x4 rotate/*, GLuint texture*/,QVector3D color)
 {
     UpdateShaders(wvp, mvp, rotate/*, texture*/, color);
     ShaderDraw(box);
 }

void RenderState::forwardmove()
{
    m_position->setX(m_position->x()+0.1f);
}

RenderState::~RenderState()
{
    delete m_program;
}
