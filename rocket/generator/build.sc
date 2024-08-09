import mill._, scalalib._

object ArcTest extends RootModule with ScalaModule {
  def scalaVersion: T[String] = T("2.13.10")
  def unmanagedClasspath = T {
    Agg.from(os.list(millSourcePath / "lib").map(PathRef(_)))
  }
  def mainClass = Some("freechips.rocketchip.diplomacy.Main")
}
