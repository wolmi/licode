node("docker builder") {
  stage "git pull"
  git url: "${env.GIT_URL}", credentialsId: '1aa2153e-dc53-43ce-abc1-b06e3fbfb43d'

  sh "git rev-parse HEAD > .git/commit-id"
  def commit_id = readFile('.git/commit-id').trim()
  println commit_id
}
