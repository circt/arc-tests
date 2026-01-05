import mill._, scalalib._

object ArcTest extends RootModule with ScalaModule {
  def scalaVersion: T[String] = T("2.13.12")
  def unmanagedClasspath = T {
    Agg.from(os.list(millSourcePath / "lib").map(PathRef(_)))
  }

  def ivyDeps = T {
    Agg(
      ivy"org.chipsalliance::chisel::6.5.0",
      ivy"org.json4s::json4s-jackson::4.0.5"
    )
  }

  def mainClass = Some("chipyard.Generator")
}
