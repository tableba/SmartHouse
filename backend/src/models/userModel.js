class User {
  constructor(id, email, passwordHash, createdAt) {
      (this.id = id),
      (this.email = email),
      (this.passwordHash = passwordHash),
      (this.createdAt = createdAt)
  }
  static create(data) {
    return new User({
      ...data,
      createdAt: new Date().toISOString()
    })
  }

  changePasswordHash(newHash) {
    this.passwordHash = newHash
  }

}

export default User;
