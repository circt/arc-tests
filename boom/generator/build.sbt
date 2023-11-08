lazy val buildSettings = Seq(
  scalaVersion := "2.13.10"
)

lazy val arc = (project in file("."))
  .settings(
    buildSettings,
    libraryDependencies ++= Seq(
      "edu.berkeley.cs" %% "chisel3" % "3.5.6",
      "org.json4s" %% "json4s-jackson" % "3.6.6"
    )
  )
