lazy val arc = (project in file("."))
  .dependsOn(RootProject(uri("git://github.com/chipsalliance/rocket-chip.git#v1.6")))
