import { useAxios } from '@vueuse/integrations/useAxios'
import axios from 'axios'
import { useMainStore } from '@/stores/main'

const API_URI = 'http://localhost:8080'

const globalClient = {
  accessToken: '',
  axios_inst: axios.create({
    baseURL: API_URI + '/api',
    withCredentials: true,
  }),
  unwrapResp(resp) {
    const data = resp.data
    const dv = data.value
    if (dv.code < 0) {
      throw new Error(`Client request failed with code ${dv.code}, '${dv.msg}'`)
    }
    return dv.data
  },
  genReqExec(path, method = 'GET') {
    const { execute } = useAxios(path, { method }, this.axios_inst, { immediate: false })
    return execute
  },
  setAccessToken(token) {
    this.accessToken = token
  },
  async authLogin(username, password) {
    const exec = this.genReqExec('/v1/auth/login', 'POST')
    return this.unwrapResp(await exec({ params: { username, password } }))
  },
  async authRegister(username, password) {
    const exec = this.genReqExec('/v1/auth/register', 'POST')
    return this.unwrapResp(await exec({ params: { username, password } }))
  },
  async authLogout() {
    const exec = this.genReqExec('/v1/auth/logout', 'POST')
    return this.unwrapResp(await exec())
  },
  async authUpdateInfo(nickname, introduction, password, sex, age, email) {
    const exec = this.genReqExec('/v1/auth/update-info', 'POST')
    return this.unwrapResp(await exec({ params: { nickname, introduction, password, sex, age, email } }))
  },
  async userGetInfo(user_id) {
    const exec = this.genReqExec('/v1/user/get-info')
    return this.unwrapResp(await exec({ params: { id: user_id } }))
  },
  async userGetInfoByName(user_name) {
    const exec = this.genReqExec('/v1/user/get-info-by-name')
    return this.unwrapResp(await exec({ params: { name: user_name } }))
  },
  async postView(post_id) {
    const exec = this.genReqExec('/v1/post/view')
    return this.unwrapResp(await exec({ params: { post_id } }))
  },
  async postList(user_id) {
    const exec = this.genReqExec('/v1/post/list')
    return this.unwrapResp(await exec({ params: { user_id } }))
  },
  async postCreate(content, reply_id) {
    const exec = this.genReqExec('/v1/post/create', 'POST')
    return this.unwrapResp(await exec({ params: { content, reply_id } }))
  },
  async postCreateForward(forward_id) {
    const exec = this.genReqExec('/v1/post/create-forward', 'POST')
    return this.unwrapResp(await exec({ params: { post_id: forward_id } }))
  },
  async postDelete(post_id) {
    const exec = this.genReqExec('/v1/post/delete', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id } }))
  },
  async postUpdate(post_id, content) {
    const exec = this.genReqExec('/v1/post/update', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id, content } }))
  },
  async postCommitAudit(post_id) {
    const exec = this.genReqExec('/v1/post/commit-audit', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id } }))
  },
  async postAuditListPending() {
    const exec = this.genReqExec('/v1/post/audit-list-pending')
    return this.unwrapResp(await exec())
  },
  async postAuditAccept(post_id) {
    const exec = this.genReqExec('/v1/post/audit-accept', 'POST')
    return this.unwrapResp(await exec({ params: { post_id } }))
  },
  async postAuditReject(post_id) {
    const exec = this.genReqExec('/v1/post/audit-reject', 'POST')
    return this.unwrapResp(await exec({ params: { post_id } }))
  },
  async postSearch(content) {
    const exec = this.genReqExec('/v1/post/search')
    return this.unwrapResp(await exec({ params: { content } }))
  },
  async collectionList() {
    const exec = this.genReqExec('/v1/collection/list')
    return this.unwrapResp(await exec())
  },
  async collectionAdd(post_id) {
    const exec = this.genReqExec('/v1/collection/add', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id } }))
  },
  async collectionRemove(post_id) {
    const exec = this.genReqExec('/v1/collection/remove', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id } }))
  },
  async likeAdd(post_id) {
    const exec = this.genReqExec('/v1/like/add', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id } }))
  },
  async likeRemove(post_id) {
    const exec = this.genReqExec('/v1/like/remove', 'POST')
    return this.unwrapResp(await exec({ params: { id: post_id } }))
  },
  async postRecommendGenerate(pn, ps) {
    const exec = this.genReqExec('/v1/post-recommend/generate')
    return this.unwrapResp(await exec({ params: { pn, ps } }))
  },
  async userRelationshipList(id) {
    const exec = this.genReqExec('/v1/user-relationship/list')
    return this.unwrapResp(await exec({ params: { id } }))
  },
  async userRelationshipListInverse(id) {
    const exec = this.genReqExec('/v1/user-relationship/list-inverse')
    return this.unwrapResp(await exec({ params: { id } }))
  },
  async userRelationshipGetTwoRelation(from, to) {
    const exec = this.genReqExec('/v1/user-relationship/get-two-relation')
    return this.unwrapResp(await exec({ params: { from, to } }))
  },
  async userRelationshipFollow(target) {
    const exec = this.genReqExec('/v1/user-relationship/follow', 'POST')
    return this.unwrapResp(await exec({ params: { target } }))
  },
  async userRelationshipBlock(target) {
    const exec = this.genReqExec('/v1/user-relationship/block', 'POST')
    return this.unwrapResp(await exec({ params: { target } }))
  },
  async userRelationshipUnfollow(target) {
    const exec = this.genReqExec('/v1/user-relationship/unfollow', 'POST')
    return this.unwrapResp(await exec({ params: { target } }))
  },

  async updateStore() {
    const mainStore = useMainStore()
    try {
      const resp = await this.userGetInfo();
      mainStore.userName = resp.name;
      mainStore.nickName = resp.nickname;
      mainStore.userId = resp.id;
      mainStore.userRights = [];
      if (resp.role == "ROLE_ADMIN" || resp.role == "ROLE_REVIEW") {
        mainStore.userRights.push('post.review');
      }
      if (resp.role == "ROLE_ADMIN") {
        mainStore.userRights.push('admin.*');
      }
    } catch (e) {
      mainStore.accessToken = ''
      mainStore.userName = '未登录'
      mainStore.nickName = '未登录'
      mainStore.userId = 0
      mainStore.userRights = []
      throw e
    }
  },
}

export const getClient = () => globalClient
